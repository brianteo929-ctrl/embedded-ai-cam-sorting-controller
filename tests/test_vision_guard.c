#include "vision_guard.h"
#include "fsm.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    const fsm_config_t config = {100, 100, 100, 20};
    sorter_fsm_t f;
    for (int cancel = 0; cancel < 2; ++cancel) {
        assert(fsm_init(&f, &config, 0));
        fsm_event(&f, EV_ARM, 0); fsm_event(&f, EV_HOME, 0);
        fsm_event(&f, EV_TRIGGER, 1);
        bool busy = true;
        assert(vision_event_current(1, 1, 1, false, &busy) && busy);
        fsm_event(&f, EV_CAPTURED, 2);
        assert(fsm_event(&f, cancel ? EV_STOP : EV_TICK, 102) == ACT_DISABLE);
        assert(!vision_event_current(1, 2, 1, false, &busy) && busy);
        assert(!vision_event_current(1, 2, 1, true, &busy) && !busy);
        assert(f.state == SORT_FAULT);
        fsm_event(&f, EV_RECOVER, 103);
        assert(!vision_event_current(1, 2, 1, true, &busy));
        assert(f.state == SORT_UNHOMED && !f.armed);
        fsm_event(&f, EV_ARM, 104); fsm_event(&f, EV_HOME, 104);
        fsm_event(&f, EV_TRIGGER, 105); busy = true;
        assert(!vision_event_current(1, 3, 3, true, &busy) && busy);
        assert(vision_event_current(3, 3, 3, false, &busy));
        fsm_event(&f, EV_CAPTURED, 106);
        assert(vision_event_current(3, 3, 3, true, &busy) && !busy);
        assert(fsm_event(&f, EV_REJECT, 107) == ACT_NONE && f.state == SORT_IDLE);
    }
    puts("PASS vision lifecycle: cancellation, timeout, recovery, stale/duplicate completion, new request isolation");
}
