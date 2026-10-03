/* SPDX-License-Identifier: LGPL-2.0-or-later */
import QtQuick.Templates as T
import org.kde.spectacle.private

T.Action {
    enabled: !SpectacleCore.videoMode
    icon.name: "window-pin"
    text: i18nc("@action", "Pin Screenshot")
    onTriggered: SpectacleCore.pinScreenshot()
}
