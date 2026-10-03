#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.0-or-later
"""Local PP-OCRv5 worker. Models are installed with the package, never fetched here."""

import argparse
import copy
import json
import statistics
import sys
import unicodedata
from pathlib import Path


def group_text_boxes(boxes, texts):
    """Group detector fragments by their visual baseline, not by OCR token order."""
    entries = []
    for index, (box, text) in enumerate(zip(boxes, texts)):
        if not text.strip():
            continue
        xs, ys = zip(*box)
        left, right, top, bottom = min(xs), max(xs), min(ys), max(ys)
        entries.append(dict(index=index, text=text.strip(), left=float(left), right=float(right),
                            top=float(top), bottom=float(bottom), center=float(top + bottom) / 2,
                            height=max(1.0, float(bottom - top))))
    groups = []
    for entry in sorted(entries, key=lambda item: (item["center"], item["left"])):
        candidates = []
        for group in groups:
            height = statistics.median(item["height"] for item in group)
            center = statistics.median(item["center"] for item in group)
            overlap = min(entry["bottom"], center + height / 2) - max(entry["top"], center - height / 2)
            if (overlap >= 0.55 * min(height, entry["height"])
                    and abs(entry["center"] - center) <= 0.4 * max(height, entry["height"])):
                candidates.append((abs(entry["center"] - center), group))
        if candidates:
            min(candidates, key=lambda item: item[0])[1].append(entry)
        else:
            groups.append([entry])

    # Split columns only after baseline grouping. A detector may report words
    # with slightly different heights in an order that temporarily leaves gaps.
    split_groups = []
    for group in groups:
        group.sort(key=lambda item: item["left"])
        current = []
        for item in group:
            if current and item["left"] - current[-1]["right"] > 4 * max(item["height"], current[-1]["height"]):
                split_groups.append(current)
                current = []
            current.append(item)
        split_groups.append(current)

    lines = []
    for group in split_groups:
        text = group[0]["text"]
        widths = []
        for index, item in enumerate(group):
            units = sum(2 if unicodedata.east_asian_width(char) in "WF" else 1 for char in item["text"])
            if units >= 4:
                widths.append((item["right"] - item["left"]) / units)
            if index:
                previous = group[index - 1]
                gap = item["left"] - previous["right"]
                cjk_boundary = all(unicodedata.east_asian_width(char) in "WF" for char in (text[-1], item["text"][0]))
                separator = "" if cjk_boundary and gap < item["height"] * 0.6 else " "
                text += separator + item["text"]
        lines.append(dict(text=text, indices=[item["index"] for item in group],
                          left=min(item["left"] for item in group), right=max(item["right"] for item in group),
                          top=min(item["top"] for item in group), bottom=max(item["bottom"] for item in group),
                          char_width=statistics.median(widths) if widths else None))
    return sorted(lines, key=lambda line: ((line["top"] + line["bottom"]) / 2, line["left"]))


def format_lines(lines):
    """Preserve visible line breaks, relative indentation and paragraph gaps."""
    if not lines:
        return ""
    origin = min(line["left"] for line in lines)
    widths = [line["char_width"] for line in lines if line["char_width"]]
    char_width = max(1.0, statistics.median(widths)) if widths else 1.0
    centers = [(line["top"] + line["bottom"]) / 2 for line in lines]
    height = statistics.median(line["bottom"] - line["top"] for line in lines)
    gaps = sorted(b - a for a, b in zip(centers, centers[1:]) if b - a >= height * 1.1)
    pitch = statistics.median(gaps[:max(1, len(gaps) // 2)]) if gaps else None
    output = []
    for index, line in enumerate(lines):
        if index and pitch:
            blank_lines = min(3, max(0, round((centers[index] - centers[index - 1]) / pitch) - 1))
            output.extend([""] * blank_lines)
        indent = min(80, max(0, round((line["left"] - origin) / char_width)))
        output.append(" " * indent + line["text"])
    return "\n".join(output)


def content_regions_from_boxes(boxes, width, height):
    """Build text lines, paragraphs and sections without recognizing their text."""
    rects = []
    for box in boxes:
        xs, ys = zip(*box)
        rect = (float(min(xs)), float(min(ys)), float(max(xs)), float(max(ys)))
        if rect[2] - rect[0] >= 8 and rect[3] - rect[1] >= 4:
            rects.append(rect)
    if not rects:
        return []
    line_height = statistics.median(rect[3] - rect[1] for rect in rects)

    def components(items, adjacent):
        parent = list(range(len(items)))

        def find(index):
            while parent[index] != index:
                parent[index] = parent[parent[index]]
                index = parent[index]
            return index

        for i, first in enumerate(items):
            for j in range(i + 1, len(items)):
                if adjacent(first, items[j]):
                    parent[find(j)] = find(i)
        groups = {}
        for index, item in enumerate(items):
            groups.setdefault(find(index), []).append(item)
        return [(min(r[0] for r in group), min(r[1] for r in group),
                 max(r[2] for r in group), max(r[3] for r in group)) for group in groups.values()]

    def same_line(a, b):
        ah, bh = a[3] - a[1], b[3] - b[1]
        overlap = min(a[3], b[3]) - max(a[1], b[1])
        gap = max(a[0] - b[2], b[0] - a[2], 0)
        return overlap >= 0.6 * min(ah, bh) and gap <= 2 * max(ah, bh)

    def same_block(a, b, gap_limit, same_font):
        if a[1] > b[1]:
            a, b = b, a
        ah, bh = a[3] - a[1], b[3] - b[1]
        aw, bw = a[2] - a[0], b[2] - b[0]
        gap = b[1] - a[3]
        if gap < -0.2 * min(ah, bh) or gap > gap_limit:
            return False
        if same_font and max(ah, bh) > 1.8 * min(ah, bh):
            return False
        overlap = max(0, min(a[2], b[2]) - max(a[0], b[0]))
        left_aligned = abs(a[0] - b[0]) <= 2 * line_height
        similar_width = min(aw, bw) >= 0.6 * max(aw, bw)
        return left_aligned or (similar_width and overlap >= 0.7 * min(aw, bw))

    lines = components(rects, same_line)
    paragraphs = components(lines, lambda a, b: same_block(a, b, 1.5 * line_height, True))
    sections = components(paragraphs, lambda a, b: same_block(a, b, 3.5 * line_height, False))
    result = []
    seen = set()
    for level, items in enumerate((lines, paragraphs, sections)):
        padding = max(3, line_height * (0.2 + level * 0.15))
        for left, top, right, bottom in items:
            if level and (left, top, right, bottom) in (lines if level == 1 else paragraphs):
                continue
            left, top = max(0, left - padding), max(0, top - padding)
            right, bottom = min(width, right + padding), min(height, bottom + padding)
            key = tuple(round(value, 2) for value in (left, top, right - left, bottom - top))
            if key not in seen:
                seen.add(key)
                result.append(list(key))
    return result


def detect_content_regions(image_path, assets, viewports):
    model = assets / "models/ch_PP-OCRv5_det_mobile.onnx"
    if not model.is_file():
        raise FileNotFoundError("Missing text detection model")
    sys.path.insert(0, str(assets / "vendor"))
    from PIL import Image
    import numpy as np
    from rapidocr import ModelType, OCRVersion
    from rapidocr.ch_ppocr_det import TextDetector
    from rapidocr.utils.parse_parameters import ParseParams
    from rapidocr.utils.log import logger

    logger.setLevel("ERROR")

    config = ParseParams.load(assets / "vendor/rapidocr/config.yaml")
    config = ParseParams.update_batch(config, {
        "Global.log_level": "error",
        "Det.ocr_version": OCRVersion.PPOCRV5,
        "Det.model_type": ModelType.MOBILE,
        "Det.model_path": str(model),
        "Det.limit_type": "max",
        "Det.limit_side_len": 2048,
        "EngineConfig.onnxruntime.intra_op_num_threads": 2,
        "EngineConfig.onnxruntime.inter_op_num_threads": 1,
    })
    config.Det.engine_cfg = config.EngineConfig[config.Det.engine_type.value]
    config.Det.model_root_dir = assets / "models"
    detector = TextDetector(config.Det)
    with Image.open(image_path) as source:
        rgba = source.convert("RGBA")
        image = Image.new("RGB", rgba.size, "white")
        image.paste(rgba, mask=rgba.getchannel("A"))
    result = []
    for x, y, w, h in viewports or [[0, 0, image.width, image.height]]:
        left, top = max(0, int(x)), max(0, int(y))
        right, bottom = min(image.width, int(x + w)), min(image.height, int(y + h))
        if right <= left or bottom <= top:
            continue
        crop = np.asarray(image.crop((left, top, right, bottom)))[:, :, ::-1].copy()
        detected = detector(crop)
        if detected.boxes is not None:
            for rx, ry, rw, rh in content_regions_from_boxes(detected.boxes, right - left, bottom - top):
                result.append([left + rx, top + ry, rw, rh])
    return result


def recognize(image_path, assets):
    models = assets / "models"
    names = {
        "Det": "ch_PP-OCRv5_det_mobile.onnx",
        "Rec": "ch_PP-OCRv5_rec_server.onnx",
        "Cls": "ch_PP-LCNet_x0_25_textline_ori_cls_mobile.onnx",
    }
    english_model = "en_PP-OCRv5_rec_mobile.onnx"
    for name in (*names.values(), english_model):
        if not (models / name).is_file():
            raise FileNotFoundError(f"Missing OCR model: {name}")

    # The pinned private wheels avoid changing the host's Python/OpenCV setup.
    # All other Python libraries and ONNX Runtime are package dependencies.
    sys.path.insert(0, str(assets / "vendor"))
    from rapidocr import LangRec, ModelType, OCRVersion, RapidOCR
    from rapidocr.ch_ppocr_rec import TextRecInput, TextRecognizer
    from rapidocr.utils.process_img import get_rotate_crop_image
    from PIL import Image

    params = {
        "Global.log_level": "error",
        "Global.text_score": 0.6,
        "Global.max_side_len": 4000,
        "EngineConfig.onnxruntime.intra_op_num_threads": 4,
        "EngineConfig.onnxruntime.inter_op_num_threads": 1,
        "Det.model_type": ModelType.MOBILE,
        "Det.limit_side_len": 1600,
        "Det.limit_type": "max",
        "Rec.model_type": ModelType.SERVER,
    }
    for stage, name in names.items():
        params[f"{stage}.ocr_version"] = OCRVersion.PPOCRV5
        params[f"{stage}.model_path"] = str(models / name)
    # Screenshot text is often 12–20 physical pixels high. Upscaling before
    # detection avoids losing small kana and thin strokes at crop boundaries.
    with Image.open(image_path) as source:
        rgba = source.convert("RGBA")
        image = Image.new("RGB", rgba.size, "white")
        image.paste(rgba, mask=rgba.getchannel("A"))
    if max(image.size) <= 2000:
        image = image.resize((image.width * 2, image.height * 2), Image.Resampling.LANCZOS)
    import numpy as np

    # RapidOCR's ndarray input uses OpenCV's BGR order.
    bgr = np.asarray(image)[:, :, ::-1].copy()
    engine = RapidOCR(params=params)
    result = engine(bgr)
    texts = list(result.txts or ())
    lines = group_text_boxes(result.boxes, texts) if texts else []
    # The multilingual recognizer has a much larger alphabet and can collapse
    # English spaces or words. Refine Latin-only lines with the English model;
    # never run an English decoder over a Chinese/Japanese/mixed-script line.
    english_indices = [i for i, line in enumerate(lines)
                       if line["text"].isascii() and sum(character.isalpha() for character in line["text"]) >= 3]
    if english_indices:
        config = copy.deepcopy(engine.cfg.Rec)
        config.lang_type = LangRec.EN
        config.model_type = ModelType.MOBILE
        config.model_path = str(models / english_model)
        crops = []
        for i in english_indices:
            line = lines[i]
            # Recognize the complete visual line so commands split into several
            # detector boxes retain their word spacing and parameter order.
            box = (result.boxes[line["indices"][0]] if len(line["indices"]) == 1 else
                   [[line["left"], line["top"]], [line["right"], line["top"]],
                    [line["right"], line["bottom"]], [line["left"], line["bottom"]]])
            crops.append(get_rotate_crop_image(bgr, np.array(box, dtype=np.float32)))
        crops = engine.text_cls(crops).img_list
        english = TextRecognizer(config)(TextRecInput(img=crops))
        for index, text, score in zip(english_indices, english.txts, english.scores):
            original_score = statistics.mean(result.scores[i] for i in lines[index]["indices"])
            if score >= 0.85 and score >= original_score - 0.02:
                lines[index]["text"] = text
    # Keep detected reading order and Unicode scripts; do not convert Han
    # characters to a preferred language or concatenate language-specific passes.
    return format_lines(lines)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--assets", type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument("--regions", action="store_true")
    parser.add_argument("--viewports", type=json.loads)
    parser.add_argument("image", type=Path)
    args = parser.parse_args()
    try:
        if args.regions:
            regions = detect_content_regions(args.image, args.assets, args.viewports)
            print(json.dumps({"version": 1, "success": True, "regions": regions}))
            return 0
        text = recognize(args.image, args.assets)
        print(json.dumps({"version": 1, "success": True, "text": text}, ensure_ascii=False))
        return 0
    except Exception as error:
        print(json.dumps({"version": 1, "success": False, "error": str(error)}, ensure_ascii=False))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
