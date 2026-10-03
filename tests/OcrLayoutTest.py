#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.0-or-later
"""Regression cases for OCR detector boxes, independent of model inference."""
import importlib.util
from pathlib import Path
import sys
import unittest

sys.dont_write_bytecode = True

spec = importlib.util.spec_from_file_location("ocr_worker", Path(sys.argv.pop(1)))
worker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(worker)


def box(x, y, width, height=16):
    return [[x, y], [x + width, y], [x + width, y + height], [x, y + height]]


class OcrLayoutTest(unittest.TestCase):
    def test_command_tokens_on_one_visual_line(self):
        words = ["qdbus6", "org.kde.spectacle", "/MainApplication", "org.qtproject.Qt.QCoreApplication.quit"]
        boxes = []
        x = 20
        for index, word in enumerate(words):
            boxes.append(box(x, 30 + index % 2, len(word) * 8))
            x += (len(word) + 1) * 8
        text = worker.format_lines(worker.group_text_boxes(boxes, words))
        self.assertEqual(text, " ".join(words))

    def test_actual_newlines_indentation_and_blank_line(self):
        words = ["qdbus6", "org.kde.spectacle", "--help", "Restarted", "Spectacle"]
        boxes = [box(20, 10, 48), box(76, 10, 128), box(52, 40, 48),
                 box(20, 100, 72), box(100, 100, 72)]
        self.assertEqual(worker.format_lines(worker.group_text_boxes(boxes, words)),
                         "qdbus6 org.kde.spectacle\n    --help\n\nRestarted Spectacle")

    def test_distant_columns_are_not_one_command(self):
        lines = worker.group_text_boxes([box(20, 10, 48), box(600, 10, 48)], ["qdbus6", "Cancel"])
        self.assertEqual(len(lines), 2)

    def test_chinese_fragments_do_not_gain_word_spaces(self):
        lines = worker.group_text_boxes([box(20, 10, 32), box(54, 10, 32)], ["截图", "文字"])
        self.assertEqual(worker.format_lines(lines), "截图文字")

    def test_detection_order_does_not_scramble_words(self):
        lines = worker.group_text_boxes([box(100, 10, 72), box(20, 10, 72)], ["Spectacle", "Restarted"])
        self.assertEqual(worker.format_lines(lines), "Restarted Spectacle")

    def test_borderless_paragraphs_and_columns(self):
        boxes = [box(20, 10, 180), box(20, 34, 160), box(20, 98, 180), box(20, 122, 150),
                 box(500, 10, 180), box(500, 34, 160)]
        regions = worker.content_regions_from_boxes(boxes, 800, 200)

        def contains(rect, x, y):
            return rect[0] <= x <= rect[0] + rect[2] and rect[1] <= y <= rect[1] + rect[3]

        # A paragraph includes the whitespace between its lines, not just glyphs.
        self.assertTrue(any(contains(r, 50, 15) and contains(r, 50, 45) and not contains(r, 50, 110) for r in regions))
        self.assertTrue(any(contains(r, 50, 15) and contains(r, 50, 130) for r in regions))
        self.assertFalse(any(contains(r, 50, 15) and contains(r, 550, 15) for r in regions))

    def test_single_text_line_has_no_redundant_block_variants(self):
        self.assertEqual(len(worker.content_regions_from_boxes([box(20, 10, 180)], 800, 200)), 1)


if __name__ == "__main__":
    unittest.main()
