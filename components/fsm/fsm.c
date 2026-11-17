#include "fsm.h"
#include <stddef.h>

static void enter(sorter_fsm_t *f, sort_state_t state, uint32_t now)
{
    f->state = state;
    f->entered_ms = now;
}
static unsigned fault(sorter_fsm_t *f, sort_fault_t reason, uint32_t now)
{
    f->fault = reason;
    f->armed = f->referenced = false;
    enter(f, SORT_FAULT, now);
    return ACT_DISABLE;
}
bool fsm_init(sorter_fsm_t *f, const fsm_config_t *c, uint32_t now)
{
    if (!f || !c || !c->capture_ms || !c->inference_ms || !c->motion_ms || !c->settle_ms ||
        c->capture_ms > INT32_MAX || c->inference_ms > INT32_MAX ||
        c->motion_ms > INT32_MAX || c->settle_ms > INT32_MAX) return false;
    *f = (sorter_fsm_t){.state = SORT_UNHOMED, .entered_ms = now, .config = *c};
    return true;
}
unsigned fsm_event(sorter_fsm_t *f, sort_event_t e, uint32_t now)
{
    if (!f) return ACT_NONE;
    if (e == EV_STOP) return fault(f, FAULT_STOP, now);
    if (e == EV_ERROR && f->state != SORT_FAULT) {
        sort_fault_t reason = f->state == SORT_CAPTURE ? FAULT_CAPTURE : f->state == SORT_INFERENCE ? FAULT_INFERENCE :
            (f->state == SORT_ACTUATE || f->state == SORT_RESET) ? FAULT_MOTION : FAULT_IO;
        return fault(f, reason, now);
    }
    switch (f->state) {
    case SORT_UNHOMED:
        if (e == EV_ARM && !f->armed) { f->armed = true; return ACT_ARM; }
        if (e == EV_HOME && f->armed) { f->referenced = true; enter(f, SORT_IDLE, now); return ACT_REFERENCE; }
        break;
    case SORT_IDLE:
        if (e == EV_TRIGGER && f->armed && f->referenced) { enter(f, SORT_CAPTURE, now); return ACT_CAPTURE; }
        break;
    case SORT_CAPTURE:
        if (e == EV_CAPTURED) enter(f, SORT_INFERENCE, now);
        else if (e == EV_TICK && now - f->entered_ms >= f->config.capture_ms) return fault(f, FAULT_CAPTURE, now);
        break;
    case SORT_INFERENCE:
        if (e == EV_ACCEPT) { enter(f, SORT_ACTUATE, now); return ACT_SORT; }
        else if (e == EV_REJECT) enter(f, SORT_IDLE, now);
        else if (e == EV_TICK && now - f->entered_ms >= f->config.inference_ms) return fault(f, FAULT_INFERENCE, now);
        break;
    case SORT_ACTUATE:
    case SORT_RESET:
        if (e == EV_MOTION_DONE) enter(f, f->state == SORT_ACTUATE ? SORT_VERIFY : SORT_IDLE, now);
        else if (e == EV_TICK && now - f->entered_ms >= f->config.motion_ms) return fault(f, FAULT_MOTION, now);
        break;
    case SORT_VERIFY:
        if (e == EV_TICK && now - f->entered_ms >= f->config.settle_ms) { enter(f, SORT_RESET, now); return ACT_RETURN; }
        break;
    case SORT_FAULT:
        if (e == EV_RECOVER) { f->fault = FAULT_NONE; enter(f, SORT_UNHOMED, now); }
        break;
    }
    return ACT_NONE;
}
const char *fsm_state_name(sort_state_t state)
{
    static const char *names[] = {"unhomed", "idle", "capture", "inference", "actuate", "verify_open_loop", "reset", "fault"};
    return (unsigned)state < 8 ? names[state] : "invalid";
}
const char *fsm_fault_name(sort_fault_t reason)
{
    static const char *names[] = {"none", "stop", "capture", "inference", "motion", "io"};
    return (unsigned)reason < 6 ? names[reason] : "invalid";
}
