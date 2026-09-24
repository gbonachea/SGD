import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

/**
 * Botón de acción redondo-cuadrado para la barra lateral de energía / control.
 * La etiqueta se dibuja debajo del icono y un tooltip se muestra al pasar el ratón.
 */
Rectangle {
    id: control

    property string kind: "power"
    property string label: "Acción"
    property color baseColor: "#313244"
    property color hoverColor: "#45475a"
    property color iconColor: "#b4befe"

    signal triggered()

    // Anchura fija: nunca debe superar la barra lateral ni desbordar el panel.
    Layout.preferredWidth: 104
    Layout.minimumWidth: 104
    Layout.maximumWidth: 104

    implicitHeight: 54
    radius: 14
    clip: true

    color: mouse.containsMouse ? hoverColor : baseColor
    border.width: 1
    border.color: mouse.containsMouse
                  ? Qt.rgba(iconColor.r, iconColor.g, iconColor.b, 0.4)
                  : "transparent"

    ToolTip.visible: mouse.containsMouse
    ToolTip.delay: 350
    ToolTip.text: control.label

    VectorIcon {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 7
        kind: control.kind
        color: control.iconColor
        implicitWidth: 20
        implicitHeight: 20
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 6
        width: parent.width - 6
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        text: control.label
        color: control.iconColor
        font.pixelSize: 9
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: control.triggered()
    }
}