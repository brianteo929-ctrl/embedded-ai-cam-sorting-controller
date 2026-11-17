#include "decision.h"
#include "fsm.h"
#include "motion.h"
#include "image.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>

static void decisions(void)
{
    const label_map_t map[] = {{"bottle", BIN_RECYCLABLE}, {"peel", BIN_ORGANIC}, {"battery", BIN_HAZARDOUS}};
    detection_t d[2] = {{"bottle", .9f}, {"peel", .9f}};
#define DECIDE(n) decide(d, (n), true, map, 3, .8f)
    assert(DECIDE(0).reason == DECISION_EMPTY);
    for (unsigned i = 0; i < 3; ++i) { d[0].label = map[i].label; assert(DECIDE(1).bin == map[i].bin); }
    d[0].label = "bottle"; assert(DECIDE(2).reason == DECISION_CONFLICT);
    d[1].label = "bottle"; assert(DECIDE(2).reason == DECISION_MULTIPLE);
    d[0].confidence = .79f; assert(DECIDE(1).reason == DECISION_LOW_CONFIDENCE);
    d[0].confidence = .8f; assert(DECIDE(1).reason == DECISION_ACCEPT);
    d[0].label = "unmapped"; assert(DECIDE(1).reason == DECISION_UNKNOWN);
    d[0].confidence = NAN; assert(DECIDE(1).reason == DECISION_INVALID);
    assert(DECIDE(17).reason == DECISION_INVALID);
    assert(decide(NULL, 1, true, map, 3, .8f).reason == DECISION_INVALID);
    assert(decide(d, 1, false, map, 3, .8f).reason == DECISION_MODEL_ERROR);
    assert(decide(d, 0, true, map, 3, NAN).reason == DECISION_INVALID);
    const label_map_t duplicate[] = {{"a", BIN_ORGANIC}, {"a", BIN_HAZARDOUS}};
    assert(decide(d, 0, true, duplicate, 2, .8f).reason == DECISION_INVALID);
    puts("PASS classification: categories, ambiguity, thresholds, invalid/model results");
}
static sorter_fsm_t ready(uint32_t now)
{
    sorter_fsm_t f;
    fsm_config_t c = {100, 200, 300, 50};
    assert(fsm_init(&f, &c, now));
    assert(fsm_event(&f, EV_TRIGGER, now) == ACT_NONE);
    assert(fsm_event(&f, EV_HOME, now) == ACT_NONE);
    assert(f.state == SORT_UNHOMED);
    assert(fsm_event(&f, EV_ARM, now) == ACT_ARM);
    assert(fsm_event(&f, EV_HOME, now) == ACT_REFERENCE);
    assert(f.state == SORT_IDLE);
    return f;
}
static void transitions(void)
{
    sorter_fsm_t f = ready(0);
    assert(fsm_event(&f, EV_TRIGGER, 1) == ACT_CAPTURE);
    assert(fsm_event(&f, EV_TRIGGER, 2) == ACT_NONE);
    assert(fsm_event(&f, EV_ACCEPT, 2) == ACT_NONE);
    fsm_event(&f, EV_CAPTURED, 3); fsm_event(&f, EV_REJECT, 4); assert(f.state == SORT_IDLE);
    fsm_event(&f, EV_TRIGGER, 5); fsm_event(&f, EV_CAPTURED, 6);
    assert(fsm_event(&f, EV_ACCEPT, 7) == ACT_SORT);
    fsm_event(&f, EV_HOME, 8); assert(f.state == SORT_ACTUATE);
    fsm_event(&f, EV_MOTION_DONE, 9); assert(f.state == SORT_VERIFY);
    assert(fsm_event(&f, EV_TICK, 58) == ACT_NONE);
    assert(fsm_event(&f, EV_TICK, 59) == ACT_RETURN);
    fsm_event(&f, EV_MOTION_DONE, 60); assert(f.state == SORT_IDLE);
    for (int state = SORT_UNHOMED; state <= SORT_FAULT; ++state) {
        f.state = (sort_state_t)state;
        assert(fsm_event(&f, EV_STOP, 70) == ACT_DISABLE);
        assert(f.state == SORT_FAULT && !f.armed && !f.referenced);
        fsm_event(&f, EV_HOME, 71); assert(f.state == SORT_FAULT);
        fsm_event(&f, EV_RECOVER, 72); assert(f.state == SORT_UNHOMED);
    }
    puts("PASS FSM: calibration, rejection, full cycle, busy commands, stop and recovery");
}
static void failures(void)
{
    const sort_state_t states[] = {SORT_CAPTURE, SORT_INFERENCE, SORT_ACTUATE, SORT_RESET};
    const unsigned timeout[] = {100, 200, 300, 300};
    const sort_fault_t reasons[] = {FAULT_CAPTURE, FAULT_INFERENCE, FAULT_MOTION, FAULT_MOTION};
    for (unsigned i = 0; i < 4; ++i) {
        sorter_fsm_t f = ready(UINT32_MAX - 50); f.state = states[i];
        assert(fsm_event(&f, EV_TICK, f.entered_ms + timeout[i] - 1) == ACT_NONE);
        assert(fsm_event(&f, EV_TICK, f.entered_ms + timeout[i]) == ACT_DISABLE);
        assert(f.fault == reasons[i]);
        f = ready(0); f.state = states[i];
        assert(fsm_event(&f, EV_ERROR, 0) == ACT_DISABLE); assert(f.fault == reasons[i]);
    }
    sorter_fsm_t f; fsm_config_t bad = {0, 1, 1, 1}; assert(!fsm_init(&f, &bad, 0));
    puts("PASS faults: capture, inference, both motion phases, timer rollover, bad config");
}
static void planning(void)
{
    motion_config_t c = {{1024, 1024}, 10000, 2000, 20, {14, 2}, {13, 12}, 15};
    int32_t from[] = {0, 0}, to[] = {100, -50}, delta[2];
    assert(motion_plan(&c, from, to, delta) && delta[0] == 100 && delta[1] == -50);
    assert(motion_duration_ms(&c, delta) > 300);
    assert(motion_period(&c, 0, 100) == 10000 && motion_period(&c, 20, 100) == 2000);
    for (unsigned n = 1; n <= MOTION_MAX_STEPS; ++n)
        for (unsigned i = 0; i < n; ++i) {
            uint32_t p = motion_period(&c, i, n);
            assert(p >= 2000 && p <= 10000 && p == motion_period(&c, n - 1 - i, n));
        }
    to[0] = INT_MAX; assert(!motion_plan(&c, from, to, delta));
    to[0] = -1024; from[0] = 1024;
    assert(motion_plan(&c, from, to, delta) && delta[0] == -2048);
    c.enable_pin = 12; assert(!motion_config_valid(&c));
    c.enable_pin = 15; c.step_pin[1] = 14; assert(!motion_config_valid(&c));
    c.step_pin[1] = 2; c.dir_pin[1] = 4; assert(!motion_config_valid(&c));
    c.dir_pin[1] = 12; c.cruise_period_us = 1; assert(!motion_config_valid(&c));
    puts("PASS motion: signed bounds, audited pins, duration, symmetric pulse ramps");
}
static void images(void)
{
    const uint8_t pixels[] = {255,0,0, 0,255,0, 0,0,255, 255,255,255};
    uint8_t out[12];
    assert(image_center_crop(pixels, 2, 2, out, 1, 1));
    assert(out[0] == 128 && out[1] == 128 && out[2] == 128);
    assert(image_center_crop(pixels, 2, 2, out, 2, 2));
    for (unsigned i = 0; i < 12; ++i) assert(pixels[i] == out[i]);
    const uint8_t strip[] = {0,0,0, 255,0,0, 0,0,0};
    assert(image_center_crop(strip, 3, 1, out, 1, 1));
    assert(out[0] == 255 && out[1] == 0 && out[2] == 0);
    assert(!image_center_crop(NULL, 2, 2, out, 1, 1));
    assert(!image_center_crop(pixels, 0, 2, out, 1, 1));
    assert(!image_center_crop(pixels, 2, 2, out, 257, 1));
    puts("PASS image: RGB ordering, identity, bilinear reduction, center crop, bounds");
}
int main(void) { decisions(); transitions(); failures(); planning(); images(); return 0; }
