import QtQuick
import QtQuick.Layouts

/**
 * Insignia pequeña [ tecla ] + leyenda, usada en la barra de ayuda inferior.
 */
RowLayout {
    id: root

    property string label: "↵"
    property string caption: ""
    property color badgeColor: "#25253a"
    property color textColor: "#a6adc8"

    spacing: 6

    Rectangle {
        id: badge
        Layout.preferredWidth: Math.max(24, badgeLabel.implicitWidth + 12)
        Layout.preferredHeight: 20
        radius: 6
        color: root.badgeColor
        border.width: 1
        border.color: "#45475a"

        Text {
            id: badgeLabel
            anchors.centerIn: parent
            text: root.label
            color: root.textColor
            font.pixelSize: 11
            font.weight: Font.DemiBold
        }
    }

    Text {
        text: root.caption
        color: root.textColor
        font.pixelSize: 11
        visible: root.caption !== ""
    }
}