"""Create deterministic local contact sheets; never approve or relabel samples."""
import argparse
import json
from pathlib import Path
import random

from PIL import Image, ImageDraw, ImageOps

from dataset import digest, safe_file, write_json


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--per-class", type=int, default=4)
    parser.add_argument("--seed", type=int, default=1168582)
    args = parser.parse_args()
    if not 1 <= args.per_class <= 16:
        raise ValueError("Choose 1 to 16 examples per raw class")
    if args.output.exists():
        raise ValueError("Use a fresh review directory")
    rows = json.loads(args.manifest.read_text(encoding="utf-8"))["records"]
    labels = sorted({box["label"] for row in rows for box in row.get("annotations", [])})
    rng, selected = random.Random(args.seed), []
    for label in labels:
        candidates = [r for r in rows if len(r.get("annotations", [])) == 1 and r["annotations"][0]["label"] == label]
        if not candidates:
            candidates = [r for r in rows if any(b["label"] == label for b in r.get("annotations", []))]
        selected.extend(rng.sample(candidates, min(args.per_class, len(candidates))))
    for predicate, count in ((lambda n: n == 0, 4), (lambda n: n > 1, 8)):
        candidates = [r for r in rows if predicate(len(r.get("annotations", [])))]
        selected.extend(rng.sample(candidates, min(count, len(candidates))))
    selected = list({row["path"]: row for row in selected}.values())
    args.output.mkdir(parents=True)
    index = []
    for start in range(0, len(selected), 16):
        batch = selected[start:start+16]
        sheet = Image.new("RGB", (1280, 292*((len(batch)+3)//4)), "white")
        draw = ImageDraw.Draw(sheet)
        for offset, row in enumerate(batch):
            file = safe_file(args.root, row["path"])
            with Image.open(file) as image:
                preview = ImageOps.exif_transpose(image).convert("RGB")
                preview.thumbnail((320, 256))
                x, y = offset % 4 * 320, offset // 4 * 292
                sheet.paste(preview, (x, y))
                labels = sorted({b["label"] for b in row.get("annotations", [])})
                draw.text((x+2, y+258), f"{start+offset}: {','.join(labels)[:40]}", fill="black")
                draw.text((x+2, y+274), f"{row['split']} / {len(row.get('annotations', []))} boxes", fill="black")
            index.append({"index": start+offset, "path": row["path"], "sha256": digest(file),
                          "observation": None, "approved": False})
        sheet.save(args.output / f"sheet-{start//16+1:02}.jpg")
    write_json(args.output / "index.json", {"seed": args.seed, "samples": index})
    print(f"Prepared {len(index)} review samples; no labels approved")


if __name__ == "__main__":
    main()
