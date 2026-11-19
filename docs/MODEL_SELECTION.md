# Model selection and architecture decision

2026-11-19: bounded review of three reusable candidates. **None selected for
deployment or used as a smoke-test model.** Public model descriptions and
repository files are evidence of availability, not independently reproduced scores.

| Candidate | Artifact, license and input/output | Resources / compatibility / disposition |
|---|---|---|
| [KrisnaSantosa15 / WasteNet](https://github.com/KrisnaSantosa15/wastenet-garbage-classifier) | MIT LICENSE present; downloadable `tflite/model.tflite`, SavedModel and Keras files. MobileNetV2-based, 224×224 RGB, 12-way classification: battery, biological, brown/green/white-glass, cardboard, clothes, metal, paper, plastic, shoes, trash. Conversion source enables TFLite builtins without integer quantization. | TFLite file **20,675,188 bytes**, far above 512 KiB. Tensor arena/ESP32 latency unknown; TFLite builtins do not prove Micro compatibility. EI supports import formats, but this artifact is unsuitable as the final MCU model; possible transfer-learning reference only after upstream data-license review. Not downloaded. |
| [Junfeng Zou / recyclable_waste_classification4, EI 676802](https://studio.edgeimpulse.com/public/676802/live) | Public dashboard declares BSD-3-Clause; classification, labels include battery/biological/cardboard/clothes/glass plus five others. Indexed dashboard reports float32 validation 82.5%, test 75.9%. Exact architecture/input/color/complete label meanings/downloadable weights were not verified. | Dashboard target is Raspberry Pi 5, reports 1.7M flash and 3 ms; neither is an ESP32 profile. MCU operators, RAM, quantization and export compatibility unverified. Candidate for later inspection/fine tuning, not usable under current access and budget evidence. No cloning or private access attempted. |
| [Waste segregation / waste-classification-e2jvj/1](https://universe.roboflow.com/waste-segregation-cxcrv/waste-classification-e2jvj) | Dataset declares CC BY 4.0; Roboflow 2.0 **multi-label** classifier, 52 mixed labels including `_multiple` variants. Hosted prediction API is advertised. Separate model-weight license and downloadable MCU weights unverified. | Architecture/input/color/quantization/ROM/arena/MCU ops/ESP32 latency unavailable. Cloud API is not an embedded artifact. Multi-label output violates the planned softmax contract. Unsuitable for direct integration. |

No candidate meets verified licensing, downloadable MCU weights, complete visual
taxonomy and resource requirements together. Missing evidence is not estimated
as zero. No ONNX/SavedModel or FOMO waste artifact with sufficient verified
metadata displaced these candidates in this bounded screen. No training ran.

## Decision record

Select **compact image classification as the next baseline experiment**, subject
to dataset audit and measured results. Assume a single centered item on a fixed
plate, fixed camera, manual trigger, stable lighting, and an enforced single-item
loading procedure. Neither location nor count is needed to choose a bin.

Classification can use reviewed whole-image labels, with lower output/firmware
complexity. FOMO would require reliable boxes and learns centroids useful for
counting/location; it remains supported if that physical assumption changes.
FOMO can separate background spatially, whereas classification needs explicit
reject examples and cannot guarantee detection of multiple or unfamiliar items.
Do not claim the classifier is an object-count safety sensor.

Begin with 96×96 grayscale, Image processing, MobileNetV1 0.25 transfer learning,
integer quantization, fit-shortest-axis resize, and class probabilities.
The [official model catalog](https://docs.edgeimpulse.com/studio/projects/learning-blocks/blocks/transfer-learning-images)
lists approximately 105.9K RAM / 301.6K ROM for this backbone, supporting a
plausible starting point under the 512 KiB limits. These are catalog figures,
**not this project's model profile**; latency is unknown. Grayscale is provisional:
compare RGB only if inspected images/confusions demonstrate a color requirement.

No accuracy advantage over FOMO has been measured. FOMO is deferred because
location/count are unnecessary and annotation quality is unverified, not because
it is proven slower or larger. Train a FOMO baseline only if those requirements
become material; preserve its independent compile-time checks. After a measured
baseline limitation, allow one smaller configuration (for example MobileNetV1
0.1), documenting per-class accuracy loss and resource changes. No broad tuning
or reinforcement learning.
