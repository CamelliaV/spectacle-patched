/*
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QRectF>

namespace SelectionRectUtils
{
[[nodiscard]] QRectF restorableSelectionRect(const QRectF &savedRect, const QRectF &screensRect);
}
