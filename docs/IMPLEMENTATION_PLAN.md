# Implementation plan

1. [x] Audit code, history, configuration, hardware constraints, and baseline.
2. [x] Add host-tested classification, motion planning, and event-driven FSM.
3. [x] Integrate asynchronous two-axis RMT, cancellation, reference handling.
4. [x] Port camera/AP into IDF; serialize frame ownership; add vision worker and model interface.
5. [x] Add bounded HTTP control/status and diagnostics.
6. [x] Verify configurations; document wiring, model, bring-up, resources, learning.
7. [x] Prepare verified implementation and dependency-hygiene commits on existing main. Final regression and remote push results are recorded in the delivery report.

Constraints: no model export or serial device; unknown motor ratings/sense resistors/mechanical positions; 12 V / 1 A supply unqualified. Sequential open-loop movement, compiled simulation by default, no assumed home at boot. No new MCP services.

Verification: native C tests, incremental IDF builds after integration changes, full regression before commits/push. Diagnose failures before edits; stop a repeated unresolved failure after three supported repairs. Retain evidence in PROJECT_STATUS.md.
