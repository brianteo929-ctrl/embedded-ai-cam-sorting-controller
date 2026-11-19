# AI model status and operating contract

**No trained application model is installed.** Existing project:
[1168582](https://studio.edgeimpulse.com/studio/1168582). Impulse/deployment version,
checksum, SDK version, selected dataset, quantization, per-class precision/recall/F1,
confusion matrix, false accepts, tensor arena and latency are all **pending**, not
zero. Browser access and GitHub sign-in now work. The existing private project is
empty, uses one label per item, and its target was independently verified as
ESP32 240 MHz / AI-Thinker ESP32-CAM, 512 KB model RAM, 512 KB model ROM and 1000 ms.
Two datasets were downloaded and audited but neither is approved for training:
upstream rights, independent splits, annotation semantics and real reject examples
remain unresolved. No model was reused or trained in this work.

See [dataset selection](DATASET_SELECTION.md), [audit workflow](DATASET_AUDIT.md)
and [architecture decision](MODEL_SELECTION.md).

## Taxonomy and rejection

The authoritative real-model mapping is
[`components/fsm/waste_labels.def`](../components/fsm/waste_labels.def):

| Destination | Exact labels |
|---|---|
| Recyclable | plastic_bottle, aluminum_can, cardboard, paper, glass_bottle |
| Organic | banana_peel, food_scraps |
| Hazardous/Special Waste | battery, light_bulb, electronics, aerosol_can |
| No actuation | background, unknown, multiple_items; all unmapped labels |

This is prototype routing policy, not proof of composition, contamination, contents,
municipal acceptance or safe handling. A mapped label is not evidence it has been
trained. Real metadata labels must be unique and known; classification requires
all three reject classes. Simulation's broad labels remain separate.

FOMO accepts exactly one non-padding detection with finite confidence at or above
threshold. Every multiple detection, including same-category objects, rejects.
Classification consumes the entire vector: unique known labels, finite [0,1]
scores, and sum within 0.02 of one (quantized-softmax numeric tolerance). Equal top
scores or an insufficient top-two margin reject. The winner must map to a bin and
meet threshold. Bin probabilities are not combined. Class probabilities describe
alternatives for one image, not separate objects.

The 80% confidence and 10-point margin are **provisional test settings**, not
validation-derived thresholds. `SORTER_MODEL_VALIDATED` defaults off and blocks
real-model physical arming. Missing/incompatible models fail configuration,
compilation or initialization. SDK errors, invalid output, timeout, cancellation
and stale results never choose a fallback bin. Simulation remains default.

## Validation and thresholds

Use object/session-separated training, explicit validation and held-out test sets
with background, unfamiliar items, hazardous confusions and multiple items.
Record dataset hashes, split counts, project/deployment version, architecture,
epochs/learning rate, augmentation, dimensions/color, quantization and target.
Export validation scores and compare coverage as confidence and top-two margin
increase. Select a useful operating point meeting documented hazardous-misrouting
and negative-frame false-actuation criteria; freeze it before test evaluation.
Report counts/sample sizes, per-class precision/recall/F1, ordinary and rejection-
aware confusion, hazardous false negatives, incorrect hazardous predictions and
known-item rejection. Zero errors in a small sample is not proof of zero risk.
If no useful operating point exists, collect data or reject the model.

Whole-image classification cannot guarantee multiple-object or unknown rejection.
Require an enforced one-item loading procedure; use sensing/detection if object
count must be verified. Studio accuracy alone cannot authorize physical control.

## Export and install

1. After dataset acceptance, train the baseline in MODEL_SELECTION.md in project
   1168582, preserving explicit validation/test splits. Visually reconcile labels
   and counts after import. No upload bundle exists yet.
2. Profile on ESP32 and export the quantized **C++ library**, with EON optimization
   if its measured profile fits. Save under ignored `build/model-artifacts/`.
3. Copy `config/model_manifest.example.json` to a release-specific manifest.
   Fill every unknown from actual evidence: archive/model SHA-256, model path,
   deployment/SDK version, resize, dimensions, channels, labels, license file,
   quantization, resource profile, validation report and thresholds.
4. Run `python tools/verify_model.py ARCHIVE MANIFEST --extract build/ei-staging-VERSION`.
   It verifies checksums, declared budgets/taxonomy and safe extraction, not license
   ownership, measured performance or SDK compatibility. Review actual licenses
   and metadata. Incomplete manifests and existing output directories are rejected.
5. Create ignored `components/edge_impulse/` from the inspected export, containing
   `edge-impulse-sdk/`, `model-parameters/`, `tflite-model/` and a component CMake
   source list using the **Espressif** port. Never compile every platform port.
   This final CMake integration needs the actual export; it is not yet verified.
   Follow the [official ESP32 guide](https://docs.edgeimpulse.com/hardware/deployments/run-cpp-espressif-esp32).
6. In `tools\idf.cmd menuconfig`, select **Waste sorter → External Edge Impulse
   image classification component** (or the independent FOMO backend). Keep motor
   hardware and model validated off. Metadata must declare project 1168582, camera
   input, 1–256 pixel dimensions, known labels, grayscale/RGB Image features and
   fit-shortest-axis resize. Do not remove checks for a mismatched export.
7. Run `npm test`, `tools\idf.cmd build size size-components` and the hardware
   regression. Compare Studio/desktop known-vector probabilities with this adapter
   on identical decoded RGB pixels before on-device tests. Fake SDK tests cannot
   substitute for a real-model build or vector test.

Commit reviewed release metadata and appropriately licensed small test vectors;
keep datasets, archives and generated SDK/model trees ignored. Never record private
download URLs. Updates repeat checksum, metadata, regression, size and camera-domain
validation. No root sdkconfig or partition change was made for this infrastructure.

## Resource accounting

Baseline binary: 890,992 bytes; 645,008 bytes free in a 1,536,000-byte partition.
See PROJECT_STATUS.md for current link results. Installed model contribution is
**0 bytes**; future model/runtime flash, tensor arena and latency remain unknown.

96×96 preprocessing uses 230,400-byte QVGA decode + 27,648-byte RGB input = **258,048
bytes of PSRAM allocations**. JPEG scratch is 3,100 bytes; concurrent preview can
allocate another 131,072 PSRAM bytes. Camera framebuffer, SDK/tensor arena, stacks
and allocator overhead are additional. Grayscale DSP does not remove the RGB
buffer. Peak memory and physical latency require export and board measurements;
catalog RAM plus static DRAM is not a measured peak.

Keep the partition table. If the real model exceeds budgets, diagnose component
sizes, then try quantization or smaller backbone/input/grayscale with accuracy
trade-offs recorded. Partition changes require a documented OTA/recovery review.
