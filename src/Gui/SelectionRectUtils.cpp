/*
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SelectionRectUtils.h"

namespace SelectionRectUtils
{
QRectF restorableSelectionRect(const QRectF &savedRect, const QRectF &screensRect)
{
    const auto normalizedSavedRect = savedRect.normalized();
    if (normalizedSavedRect.isEmpty() || screensRect.isEmpty()) {
        return {};
    }

    return normalizedSavedRect.intersected(screensRect.normalized());
}
}
