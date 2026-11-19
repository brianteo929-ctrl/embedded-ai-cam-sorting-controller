# Dataset selection — 2026-11-19

No dataset is approved for final training. Five candidates were screened and
the best two were downloaded through authenticated Roboflow access. Both received
full programmatic audits plus stratified visual sampling; neither passed. They
were not merged or uploaded to Edge Impulse. See DATASET_AUDIT.md and the compact
machine-readable reports under `docs/reports/`. The remaining three were screened
from public project metadata only; the two-candidate audit limit is reached.

| Rank / candidate | License, version and published data | Decision |
|---|---|---|
| 1. [Material Identification / GARBAGE CLASSIFICATION 3](https://universe.roboflow.com/material-identification/garbage-classification-3/dataset/2) | CC BY 4.0; v2, 10,464 bounding-box images; train 7,324 / validation 2,098 / test 1,042; 416×416 stretch. Biodegradable, cardboard, glass, metal, paper, plastic. | Downloaded and audited. Stock-watermarked examples leave upstream rights unresolved. Broad plastic/metal/glass cannot be renamed bottle/aluminum-can; sampled boxes include component-level labels. No verified background or special-waste class. Not accepted for training. |
| 2. [battery detection / Waste-battery-only](https://universe.roboflow.com/battery-detection-yuv0z/waste-battery-only/dataset/12) | CC BY 4.0; v12, 753 bounding-box images; 597 / 156 / **0**; 640×640 with white padding, no augmentation. Alkaline, Alkaline_jp, CR_Lithium, Lithium, Lithium_jp, LR_Alkaline, Manganese, Manganese_jp. | Downloaded and audited. Sampled boxes mark chemistry text, not full batteries. All four sampled zero-box images contain batteries; repeated physical objects cross splits. Whole-image reannotation and object/session resplit are required. Not accepted for training. |
| 3. [Waste segregation / waste classification](https://universe.roboflow.com/waste-segregation-cxcrv/waste-classification-e2jvj/dataset/1) | CC BY 4.0; v1, 6,152 source images → 14,810 generated; 12,988 / 1,210 / 612; 640×640 stretch, flips/rotation and partial grayscale. | 52 multi-label classes include background/empty/multiple, bottles/paper/cans and e-waste, but inconsistent spellings and no clear food coverage. Do not turn multi-label annotations into a single class automatically. |
| 4. [Waste Classification / waste-classification-4j3md](https://universe.roboflow.com/waste-classification-bweuv/waste-classification-4j3md) | v3 advertises 25,556 generated images from 15,046 originals. Eight labels: ewaste, food_waste, leaf_waste, metal_cans, paper_waste, plastic_bags, plastic_bottles, wood_waste. **License not verified on inspected page.** | Relevant visual categories, but excluded until redistribution/training permission is established. No download or use. |
| 5. [waste classification / WasteClassification01](https://universe.roboflow.com/waste-classification-s6ei0/wasteclassification01) | CC BY 4.0; v1, 2,218 originals, 5,325 generated; organic/inorganic classification. | Too broad; inorganic mixes recyclable and special waste. Not a defensible three-destination classifier. |

Actual counts, annotation issues and duplication findings for the first two are in
DATASET_AUDIT.md. Per-class counts, box quality, negative coverage, pose, lighting,
occlusion and leakage remain unverified for candidates 3-5. Published splits do
not establish independent physical objects. None provides verified light-bulb or
aerosol training coverage; a firmware mapping does not imply a trained class.

CC BY 4.0 candidates can be considered together with attribution and a record of
transformations. Keep source author, URL, version, license and archive hash per
source; inspect upstream provenance before distributing derived data. A project's
declared license is not a visual or annotation-quality guarantee. No incompatible
or unknown-license dataset has been combined.

The two downloaded COCO archives remain under ignored `datasets/`, with checksums
in `tools/data/sources.json`. Download used **Download dataset > COCO JSON >
Download zip to computer**; no token-bearing URL or API snippet was retained.
Do not import the raw exports. The next defensible input is original, authorized
plate imagery with reject cases and object/session identity (DATA_COLLECTION.md).
Watermarks alone do not prove infringement, but a project-level CC BY declaration
without an upstream grant does not resolve the observed provenance concern under
the requirement to avoid unclear licenses. No questionable images were trained on.
