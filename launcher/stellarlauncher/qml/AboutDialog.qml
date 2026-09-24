import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

/**
 * Diálogo "Acerca de Stellar": muestra la información del aplicativo
 * (versión, Qt, sistema, arquitectura y aplicaciones detectadas).
 *
 * Se muestra como overlay dentro de la ventana principal, de modo que el
 * lanzador no se cierre al perder la activación (ver onActiveChanged en
 * main.qml): al ser un diálogo modal en la misma ventana, nunca se pierde
 * el foco de la ventana raíz.
 */

Rectangle {
    id: dialog

    // Información inyectada desde main.qml.
    property string appName: "Stellar"
    property string appSubtitle: "Lanzador y Control de Energía"
    property string appVersion: ""
    property string qtVersion: ""
    property string osName: ""
    property string arch: ""
    property int appCount: 0

    // Filas de información que se muestran en el cuerpo del diálogo.
    property var infoRows: [
        { label: qsTr("Versión"),     value: dialog.appVersion },
        { label: qsTr("Qt"),          value: dialog.qtVersion },
        { label: qsTr("Sistema"),     value: dialog.osName },
        { label: qsTr("Arquitectura"), value: dialog.arch }
    ]

    signal closed()

    // Paleta Catppuccin Mocha (idéntica a la de main.qml).
    property color cMantle: "#181825"
    property color cSurface0: "#313244"
    property color cSurface1: "#45475a"
    property color cOverlay0: "#6c7086"
    property color cText: "#cdd6f4"
    property color cSubtext: "#a6adc8"
    property color cLavender: "#b4befe"
    property color cRed: "#f38ba8"

    width: 420
    height: content.implicitHeight + 48
    radius: 20
    color: cMantle
    border.width: 1
    border.color: cSurface1

    // Atrapa los clics dentro de la tarjeta para que no se propaguen al fondo
    // (el scrim de main.qml) y cierren el diálogo por accidente.
    MouseArea { anchors.fill: parent }

    ColumnLayout {
        id: content
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 24
        spacing: 0

        // Cabecera: logo + botón de cierre
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            spacing: 10

            Rectangle {
                Layout.preferredWidth: 44
                Layout.preferredHeight: 44
                radius: 12
                color: cSurface0
                VectorIcon {
                    anchors.centerIn: parent
                    kind: "sparkle"
                    color: cLavender
                    implicitWidth: 24
                    implicitHeight: 24
                }
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                radius: 8
                color: aboutCloseMouse.containsMouse ? cSurface1 : "transparent"
                VectorIcon {
                    anchors.centerIn: parent
                    kind: "close"
                    color: aboutCloseMouse.containsMouse ? cRed : cOverlay0
                    implicitWidth: 12
                    implicitHeight: 12
                }
                MouseArea {
                    id: aboutCloseMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: dialog.closed()
                }
            }
        }

        Text {
            Layout.topMargin: 16
            text: dialog.appName
            color: cText
            font.pixelSize: 21
            font.weight: Font.DemiBold
        }
        Text {
            Layout.topMargin: 3
            text: dialog.appSubtitle
            color: cSubtext
            font.pixelSize: 12
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.topMargin: 16
            color: cSurface0
        }

        // Filas de información
        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: 16
            spacing: 10

            Repeater {
                model: dialog.infoRows
                delegate: RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    Text {
                        Layout.preferredWidth: 120
                        text: modelData.label
                        color: cSubtext
                        font.pixelSize: 12
                    }
                    Text {
                        Layout.fillWidth: true
                        text: modelData.value
                        color: cText
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                visible: dialog.appCount > 0
                spacing: 12
                Text {
                    Layout.preferredWidth: 120
                    text: qsTr("Aplicaciones")
                    color: cSubtext
                    font.pixelSize: 12
                }
                Text {
                    Layout.fillWidth: true
                    text: dialog.appCount + " " + qsTr("aplicaciones indexadas")
                    color: cLavender
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignRight
                }
            }
        }

        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            Layout.topMargin: 20
            text: qsTr("Cerrar")
            hoverEnabled: true

            contentItem: Text {
                text: parent.text
                color: "#1e1e2e"
                font.pixelSize: 13
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                radius: 10
                color: parent.hovered ? "#c3cdff" : cLavender
            }

            onClicked: dialog.closed()
        }
    }
}