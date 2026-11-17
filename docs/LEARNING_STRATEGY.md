# Learning strategy

Use supervised FOMO detection with deterministic control for this fixed three-bin sorter. Model uncertainty must not change safety limits or the motion sequence.

Collect low-confidence, rejected and misclassified scenes during supervised bench use through `/capture`; obtain consent for any incidental personal imagery and retain only what is needed. Save the true label, lighting, camera geometry, model version and reason code alongside the image. The firmware does not automatically upload or retain images.

Review annotations in Roboflow, track dataset versions and provenance, and group related views before splitting. Freeze an untouched test set. Retrain in Edge Impulse, compare per-class precision/recall, unknown-object false acceptance and multiple-object rejection. Compare quantized and float variants for accuracy, latency, flash, tensor arena, peak internal heap and PSRAM. Use measured results to choose thresholds and model size.

Version model exports with checksums and the firmware/configuration that used them. Retain a previously validated binary/export for rollback by reflashing; OTA rollback is not currently implemented. An active-learning loop can prioritize uncertain examples for human labeling, followed by offline retraining and supervised validation. There is no on-device weight update or automatic deployment.

Reinforcement learning is not justified for the present deterministic plate positions. A future simulator could expose plate angle, object pose, measured load and verified bin state; actions could select bounded trajectories, with rewards for verified correct delivery and penalties for jams, time and energy. This would require reliable sensors, a validated mechanics simulator, constrained action limits and reproducible comparisons against conventional control. Training on real uncertain waste or letting an exploratory policy operate the mechanism would add risk without a demonstrated benefit. Keep such research separate from the production controller.
