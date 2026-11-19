# Dataset audit - 2026-11-19

**Both selected candidates were acquired and audited; neither is approved for
training or imported into Edge Impulse.** Browser access is resolved. Programmatic
checks cover every image; visual findings are stratified samples, not a claim
that all 11,217 images were manually relabeled. The two-candidate audit limit is
reached. Original camera-domain data is the next input (DATA_COLLECTION.md).

Machine-readable aggregates: [garbage v2](reports/garbage_v2.json),
[battery v12](reports/battery_v12.json), and [visual evidence](reports/visual_review.json).
Full image/hash/issue indexes remain local under ignored `datasets/`.

| Check | Garbage Classification 3 v2 | Waste-battery-only v12 |
|---|---:|---:|
| Declared license | CC BY 4.0 | CC BY 4.0 |
| ZIP bytes | 228,369,116 | 18,714,074 |
| Images decoded / corrupt or missing | 10,464 / 0 | 753 / 0 |
| Resolution | 416x416 stretched | 640x640 padded |
| Train / validation / test | 7,324 / 2,098 / 1,042 | 597 / 156 / **0** |
| Zero / one / multiple annotations per image | 0 / 5,081 / 5,383 | 22 / 446 / 285 |
| Exact file / decoded duplicate groups | 0 / 0 | 0 / 0 |
| dHash near pairs / cross-split pairs | 10,281 / 5,064 | 3,315 / 1,067 |
| Out-of-bounds / tiny boxes | 17 / 2,268 | 0 / 40 |
| Overlapping box pairs (IoU > 0.8) | 177 | 0 |
| Images awaiting object/session review | 10,464 | 753 |

Checksums are recorded in `tools/data/sources.json` and the reports. They identify
the downloaded archives; they are not publisher-signed checksums. Near pairs use
dHash distance <=4 and require inspection, not automatic deletion. Four inspected
battery pairs visibly repeat the same objects across training/validation. Of four
inspected garbage pairs, three are silhouette/hash false positives and one is a
near-copy of the same cardboard-box photograph across splits. Thus both leakage
and heuristic false positives are evidenced; the pair counts are not counts of
confirmed duplicate objects.

## Raw classes and split coverage

Images can carry multiple raw labels, so image counts below do not sum to dataset
size. Annotation counts are boxes, not necessarily separate physical objects.
Full per-class split image counts are in the JSON reports.

| Garbage raw label | Images | Boxes | Test images |
|---|---:|---:|---:|
| BIODEGRADABLE | 2,287 | 45,407 | 8 |
| CARDBOARD | 1,502 | 4,698 | 11 |
| GLASS | 2,684 | 7,809 | **0** |
| METAL | 1,898 | 5,841 | 172 |
| PAPER | 1,768 | 4,390 | 515 |
| PLASTIC | 1,464 | 5,945 | 354 |

The single-annotation subset is also unsuitable as a ready classifier split:
biodegradable and glass have no single-annotation test images, plastic has no
single-annotation validation images, and paper has only two such validation
images. Source augmentation includes flips/90-degree rotations. Filenames do not
establish original identity; augmentation and repeated product shots can leak.

| Battery raw label | Images | Text-region boxes |
|---|---:|---:|
| Alkaline | 102 | 102 |
| Alkaline_jp | 300 | 336 |
| CR_Lithium | 130 | 138 |
| LR_Alkaline | 45 | 45 |
| Lithium | 194 | 205 |
| Lithium_jp | 66 | 76 |
| Manganese | 29 | 42 |
| Manganese_jp | 125 | 166 |

All eight labels may map to candidate `battery` only after whole-image review;
firmware must not infer chemistry. Garbage labels remain unmapped pending visual
relabeling: generic plastic is not necessarily a bottle, metal may be foil or a
steel can, glass may be a jar or shard, and biodegradable includes fresh produce.
`CARDBOARD`/`PAPER` spelling normalization alone would not repair provenance,
object identity, contamination or split issues. The central application mapping
remains `components/fsm/waste_labels.def`.

## Visual findings and acceptance decision

Deterministic sheets cover 32 garbage and 20 battery images across raw classes,
annotation cardinalities and source splits. Box overlays were inspected on four
examples from each source. Local review images are under `build/audit/`; no dataset pixels
are committed or redistributed.

- Battery: boxes surround chemistry words/symbols, sometimes multiple regions of
  one casing. All four sampled zero-annotation images still contain batteries.
  Treating them as background would teach unsafe negative labels. White tabletop
  views, repeated batteries, damaged casings and some product photos dominate the
  sample. There is no held-out test set or verified empty-scene coverage.
- Garbage: sampled views mix overhead waste, tightly cropped material, studio
  products, stock food photography and multi-item arrangements. Some bottle caps
  and jar lids are separately labeled; a box and its detached lid have two boxes.
  Orientation/lighting/background diversity exists, but it is not the fixed-plate
  domain and can become a class shortcut. Occlusion and small dense food boxes
  need review. Many plastic/metal/glass examples cannot support fine label names.
- Several garbage samples show stock-agency watermarks/credit strips. The archive
  declares CC BY 4.0, but no upstream grant covering those photographs was found
  in the export. This is an unresolved rights issue, not a legal finding. Under
  the requested no-unclear-license rule they cannot be training inputs. Excluding
  only visible watermarks would not establish rights for the remaining photos.
- Neither candidate supplies verified empty-plate or unfamiliar-object reject
  examples, independently grouped across all splits. No synthetic background,
  fabricated bounding box, auto-approved review flag or fabricated metric was used.

**Stop before import/training.** Missing original rights/provenance and actual
plate/negative imagery cannot be repaired from these exports alone. Whole-image
reannotation and conservative grouping could recover a limited battery subset,
but would not complete the application dataset. Keep all motors simulated and
model validation disabled. Supply original, authorized photos following
DATA_COLLECTION.md to continue in existing project 1168582.

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
Known-source imports also compare against `tools/data/sources.json`. A newly
generated Roboflow ZIP may have different bytes; review and record that new
export rather than disabling the check or claiming it is the audited archive.
The archive argument hashes provenance; it does not establish that an extracted
directory is unmodified. The report separately hashes every image and decoded
pixel buffer. Retain the downloaded annotation files and review manifest in the
versioned local artifact package.

Import does not invent boxes. It checks IDs/references and retains source boxes
and declared dimensions. Exactly one annotation supplies only a candidate class.
Zero or multiple annotations leave the image label unset; annotation count does
not establish physical item count. Each record starts with `reviewed: false`
and no `group`, so it cannot immediately enter training.

Review every image/annotation and assign a stable physical-object or collection-
session `group`, its intended split, an exact canonical label, and `reviewed: true`.
If source identity is unavailable, group conservative near-duplicate/source
clusters and document the residual uncertainty. Do not infer distinct objects
from distinct filenames. Exclude bad/missing annotations rather than treating
them as background. Do not rename generic plastic to plastic_bottle, metal to
aluminum_can, or biodegradable to food_scraps without image-level review.

The audit reports corrupt/missing/unsafe paths, unsupported labels, dimensions,
nonfinite/out-of-bounds/tiny boxes, overlapping boxes (IoU > 0.8), exact and decoded
duplicates, dHash near duplicates, per-class and per-split counts, imbalance, and
cross-split object-
group/near-duplicate leakage. Tiny boxes below 0.1% of image area require review.
dHash distance ≤4 is a conservative review heuristic; rotations, repeated objects
and semantic duplicates can evade it. Visual pose/occlusion/lighting/background
quality is not inferred by the script. Broad-domain representativeness and class
coverage remain manual acceptance requirements even after a clean audit.

Resolve flagged records and preserve a genuinely held-out test set. For a
reviewed, complete application dataset (not either rejected raw export), run:

```bat
.venv\Scripts\python tools\data\dataset.py prepare datasets\curated datasets\curated-manifest.json --output datasets\ei-curated
.venv\Scripts\python tests\test_dataset.py
```

Prepare reruns the audit, refuses unresolved issues/existing output, and emits
RGB PNGs with `label.sha256.png` names in `training/`, `validation/`, `test/` plus
provenance and audit JSON. It also requires a waste class plus `background`,
`unknown`, `multiple_items`, and every class represented in all three splits.
This minimum check is not statistical adequacy or model acceptance. It preserves
geometry; Studio and firmware perform the selected center crop. It neither merges
datasets nor fabricates annotations.
Keep all exports under ignored `datasets/`.

In existing project 1168582 enable **Data acquisition → advanced settings →
explicit validation set**, then use **Data acquisition → Upload data → Select a
folder** for each split separately, using its matching category and filename
labels. Never choose an automatic resplit. These controls are described in the
[official uploader guide](https://docs.edgeimpulse.com/studio/projects/data-acquisition/dataset/uploader).
Verify every class visually and reconcile counts after upload. Classification
requires reviewed background, unfamiliar objects and multiple-item examples in
addition to waste classes. No prepared upload folder exists yet.

To reproduce compact reports and deterministic review sheets after the audit:

```bat
.venv\Scripts\python tools\data\dataset.py summarize datasets\battery-audit.json datasets\battery-manifest.json --output docs\reports\battery_v12.json
.venv\Scripts\python tools\data\review.py datasets\battery-v12 datasets\battery-manifest.json --output build\audit\battery-review-new --per-class 1
.venv\Scripts\python tools\data\review.py datasets\garbage-v2 datasets\garbage-manifest.json --output build\audit\garbage-review-new
```

For garbage conversion use source `garbage_v2` and `datasets/garbage-v2.zip`;
no blanket mapping is supplied. Review sheets never change labels or approval
flags. Both raw audits intentionally return a nonzero acceptance status. Only the
six offline fixture tests are expected to pass; failing dataset acceptance is the
correct result for these inputs.
