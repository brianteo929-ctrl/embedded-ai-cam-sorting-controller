# Dataset audit

**Blocked before acquisition.** `data/dataset_audit.json` is a machine-readable
status report, not a completed image audit. No archive checksum or class count
has been invented. The two queued candidates are in `tools/data/sources.json`.

## Reproducible local workflow

Use a separate Python environment from ESP-IDF:

```bat
python -m venv .venv
.venv\Scripts\python -m pip install -r tools\data\requirements.txt
.venv\Scripts\python tools\data\dataset.py checksum datasets\battery-v12.zip
.venv\Scripts\python tools\data\dataset.py import-coco datasets\battery-v12 --source battery_v12 --archive datasets\battery-v12.zip --mapping tools\data\battery_labels.json --output datasets\battery-manifest.json
.venv\Scripts\python tools\data\dataset.py audit datasets\battery-v12 datasets\battery-manifest.json --output datasets\battery-audit.json
```

Extract the downloaded archive into the named local directory first. The COCO
layout is `train|valid|test/_annotations.coco.json` beside the images. Record the
initial SHA-256, then use `checksum ARCHIVE --expected HASH` on subsequent copies.
The archive argument hashes provenance; it does not establish that an extracted
directory is unmodified. The report separately hashes every image and decoded
pixel buffer. Retain the downloaded annotation files and review manifest in the
versioned local artifact package.

Import does not invent boxes. It checks IDs/references and retains source boxes
and declared dimensions. One annotated object supplies a candidate class; empty
and multi-object annotations become candidate reject classes. Each record starts
with `reviewed: false` and no `group`, so it cannot immediately enter training.

Review every image/annotation and assign a stable physical-object or collection-
session `group`, its intended split, an exact canonical label, and `reviewed: true`.
If source identity is unavailable, group conservative near-duplicate/source
clusters and document the residual uncertainty. Do not infer distinct objects
from distinct filenames. Exclude bad/missing annotations rather than treating
them as background. Do not rename generic plastic to plastic_bottle, metal to
aluminum_can, or biodegradable to food_scraps without image-level review.

The audit reports corrupt/missing/unsafe paths, unsupported labels, dimensions,
nonfinite/out-of-bounds/tiny boxes, exact and decoded duplicates, dHash near
duplicates, per-class and per-split counts, imbalance, and cross-split object-
group/near-duplicate leakage. Tiny boxes below 0.1% of image area require review.
dHash distance ≤4 is a conservative review heuristic; rotations, repeated objects
and semantic duplicates can evade it. Visual pose/occlusion/lighting/background
quality is not inferred by the script. Broad-domain representativeness and class
coverage remain manual acceptance requirements even after a clean audit.

Resolve flagged records and preserve a genuinely held-out test set, then:

```bat
.venv\Scripts\python tools\data\dataset.py prepare datasets\battery-v12 datasets\battery-manifest.json --output datasets\ei-battery-v12
.venv\Scripts\python tests\test_dataset.py
```

Prepare reruns the audit, refuses unresolved issues/existing output, and emits
RGB PNGs with `label.sha256.png` names in `training/`, `validation/`, `test/` plus
provenance and audit JSON. It preserves geometry; Studio and firmware perform
the selected center crop. It neither merges datasets nor fabricates annotations.
Keep all exports under ignored `datasets/`.

In existing project 1168582 enable **Data acquisition → advanced settings →
explicit validation set**, then use **Data acquisition → Upload data → Select a
folder** for each split separately, using its matching category and filename
labels. Never choose an automatic resplit. These controls are described in the
[official uploader guide](https://docs.edgeimpulse.com/studio/projects/data-acquisition/dataset/uploader).
Verify every class visually and reconcile counts after upload. Classification
requires reviewed background, unfamiliar objects and multiple-item examples in
addition to waste classes. No prepared upload folder exists yet.
