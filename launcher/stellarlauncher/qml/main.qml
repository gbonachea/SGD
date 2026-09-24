import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Window

/**
 * Stellar — lanzador de aplicaciones + menú de control (estilo Rofi).
 *
 * Panel izquierdo: búsqueda de aplicaciones en tiempo real (teclado + ratón).
 * Panel derecho: acciones rápidas de energía / sesión / configuración.
 *
 * Objetos del backend inyectados desde main.cpp:
 *   appModel          -> AppModel            (launchApp, datos del modelo)
 *   appProxy          -> FilterProxyModel    (modelo filtrado y ordenado)
 *   systemController  -> SystemController    (acciones de energía / sesión)
 */
Window {
    id: root
    objectName: "stellarLauncher"

    width: 700
    height: 560
    visible: true

    // Cierre seguro: la ventana se cierra cuando pierde la activación (el
    // usuario hizo clic en otra ventana / el panel / el escritorio). Solo se
    // usan eventos de foco — a diferencia de Qt::Popup, nunca toma un *grab*
    // de entrada, por lo que no puede bloquear el resto del sistema.
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"

    // Guarda para que el lanzador no se cierre solo al arrancar cuando se
    // invoca sin llegar a ser la ventana activa (p. ej. mientras otra ventana
    // a pantalla completa tiene el foco).
    property bool hasBeenActive: false

    // Un clic en cualquier lugar fuera del lanzador lo cierra: una vez que la
    // ventana ha estado activa, perder la activación significa que el usuario
    // hizo clic en otro sitio.
    onActiveChanged: {
        if (active)
            hasBeenActive = true
        else if (hasBeenActive)
            root.close()
    }

    // ---------------- Paleta (Catppuccin Mocha) ----------------
    property color cBase: "#1e1e2e"
    property color cMantle: "#181825"
    property color cCrust: "#11111b"
    property color cSurface0: "#313244"
    property color cSurface1: "#45475a"
    property color cSurface2: "#585b70"
    property color cOverlay0: "#6c7086"
    property color cText: "#cdd6f4"
    property color cSubtext: "#a6adc8"
    property color cLavender: "#b4befe"
    property color cBlue: "#89b4fa"
    property color cGreen: "#a6e3a1"
    property color cYellow: "#f9e2af"
    property color cRed: "#f38ba8"
    property color cMauve: "#cba6f7"

    property int lastLaunchTime: 0

    // ---------------- Ayudas ----------------

    function launchAt(index) {
        if (index < 0 || index >= appProxy.count)
            return
        // Antirrebote: un doble clic no debe lanzar la aplicación dos veces.
        const now = Date.now()
        if (now - lastLaunchTime < 300)
            return
        lastLaunchTime = now

        const exec = appProxy.execAt(index)
        if (!exec)
            return
        appModel.launchApp(exec)
        root.close()
    }

    function launchSelected() {
        launchAt(listView.currentIndex)
    }

    function navigate(delta) {
        if (listView.count === 0)
            return
        const index = Math.max(0, Math.min(listView.count - 1,
                                            listView.currentIndex + delta))
        listView.currentIndex = index
        listView.positionViewAtIndex(index, ListView.Contain)
    }

    function pageNav(delta) {
        const rows = Math.max(1, Math.floor(listView.height / 66))
        navigate(delta * rows)
    }

    Shortcut {
        sequence: "Escape"
        onActivated: {
            // Si el diálogo "Acerca de" está abierto, Esc lo cierra primero.
            if (aboutOverlay.opacity > 0)
                aboutOverlay.close()
            else
                root.close()
        }
    }

    // Centra la ventana en su pantalla y restaura el foco del teclado.
    onVisibleChanged: {
        if (visible) {
            x = Math.round((Screen.width - width) / 2)
            y = Math.round((Screen.height - height) / 2)
            listView.currentIndex = 0
            searchField.forceActiveFocus()
        }
    }

    Component.onCompleted: searchField.forceActiveFocus()

    // ---------------- sombra desenfocada simulada (anillos suaves) ----------------
    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        radius: 30
        color: "transparent"
        border.width: 22
        border.color: "#12000000"
    }
    Rectangle {
        anchors.fill: parent
        anchors.margins: 6
        radius: 28
        color: "transparent"
        border.width: 20
        border.color: "#2e000000"
    }

    // ---------------- panel flotante ----------------
    Rectangle {
        id: panel
        anchors.fill: parent
        anchors.margins: 26
        radius: 22
        color: cBase
        border.width: 1
        border.color: "#2e3347"
        opacity: 0
        scale: 0.97

        Component.onCompleted: fadeIn.start()

        ParallelAnimation {
            id: fadeIn
            NumberAnimation { target: panel; property: "opacity"; to: 1; duration: 180; easing.type: Easing.OutCubic }
            NumberAnimation { target: panel; property: "scale"; to: 1; duration: 180; easing.type: Easing.OutCubic }
        }

        // Franja de arrastre: mueve la ventana flotante con el ratón.
        Rectangle {
            id: dragStrip
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 40
            color: "transparent"

            MouseArea {
                anchors.fill: parent
                property point pressPos: Qt.point(0, 0)
                onPressed: (mouse) => pressPos = Qt.point(mouse.x, mouse.y)
                onPositionChanged: (mouse) => {
                    if (pressed) {
                        root.x += mouse.x - pressPos.x
                        root.y += mouse.y - pressPos.y
                    }
                }
            }
        }

        // Barra de acento en la parte superior.
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: 2
            anchors.horizontalCenter: parent.horizontalCenter
            width: 220
            height: 3
            radius: 2
            gradient: Gradient {
                GradientStop { position: 0.00; color: "transparent" }
                GradientStop { position: 0.50; color: cLavender }
                GradientStop { position: 1.00; color: "transparent" }
            }
        }

        RowLayout {
            anchors.fill: parent
            spacing: 0

            // ================= PANEL PRINCIPAL =================
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.topMargin: 16
                Layout.bottomMargin: 14
                spacing: 12

                // Cabecera
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    // Logo (estrella): al hacer clic abre "Acerca de Stellar".
                    Rectangle {
                        Layout.preferredWidth: 34
                        Layout.preferredHeight: 34
                        radius: 10
                        color: aboutMouse.containsMouse ? cSurface1 : cSurface0
                        VectorIcon {
                            anchors.centerIn: parent
                            kind: "sparkle"
                            color: aboutMouse.containsMouse ? cText : cLavender
                            implicitWidth: 20
                            implicitHeight: 20
                        }
                        ToolTip.visible: aboutMouse.containsMouse
                        ToolTip.delay: 400
                        ToolTip.text: qsTr("Acerca de Stellar")
                        MouseArea {
                            id: aboutMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: aboutOverlay.open()
                        }
                    }

                    ColumnLayout {
                        spacing: 0
                        Text {
                            text: qsTr("Stellar")
                            color: cText
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                        }
                        Text {
                            text: qsTr("Lanzador y Control de Energía")
                            color: cSubtext
                            font.pixelSize: 11
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        id: closeButton
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 30
                        radius: 9
                        color: closeMouse.containsMouse ? cSurface1 : "transparent"
                        VectorIcon {
                            anchors.centerIn: parent
                            kind: "close"
                            color: closeMouse.containsMouse ? cRed : cOverlay0
                            implicitWidth: 12
                            implicitHeight: 12
                        }
                        MouseArea {
                            id: closeMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.close()
                        }
                    }
                }

                // Campo de búsqueda
                Rectangle {
                    id: searchBox
                    Layout.fillWidth: true
                    Layout.preferredHeight: 46
                    radius: 13
                    color: cSurface0
                    border.width: 1.2
                    border.color: searchField.activeFocus ? cLavender : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 12
                        spacing: 10

                        VectorIcon {
                            kind: "search"
                            color: searchField.activeFocus ? cLavender : cOverlay0
                            implicitWidth: 17
                            implicitHeight: 17
                        }

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            TextField {
                                id: searchField
                                anchors.fill: parent
                                background: Item {}
                                color: cText
                                font.pixelSize: 15
                                selectByMouse: true
                                verticalAlignment: TextInput.AlignVCenter

                                onTextChanged: appProxy.setFilterText(text)

                                Keys.onDownPressed: root.navigate(1)
                                Keys.onUpPressed: root.navigate(-1)
                                Keys.onReturnPressed: root.launchSelected()
                                Keys.onEnterPressed: root.launchSelected()
                                Keys.onPressed: (event) => {
                                    if (event.key === Qt.Key_PageDown) {
                                        root.pageNav(1)
                                        event.accepted = true
                                    } else if (event.key === Qt.Key_PageUp) {
                                        root.pageNav(-1)
                                        event.accepted = true
                                    }
                                }
                            }

                            // Marcador de posición dibujado a mano (evita
                            // diferencias de color del placeholder entre
                            // versiones de Qt).
                            Text {
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                visible: searchField.text === "" && !searchField.activeFocus
                                text: qsTr("Buscar aplicaciones…")
                                color: cOverlay0
                                font.pixelSize: 15
                            }
                        }

                        // Botón de limpiar
                        Rectangle {
                            Layout.preferredWidth: 22
                            Layout.preferredHeight: 22
                            radius: 11
                            visible: searchField.text !== ""
                            color: clearMouse.containsMouse ? cSurface1 : "transparent"
                            VectorIcon {
                                anchors.centerIn: parent
                                kind: "close"
                                color: cOverlay0
                                implicitWidth: 10
                                implicitHeight: 10
                            }
                            MouseArea {
                                id: clearMouse
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    searchField.clear()
                                    searchField.forceActiveFocus()
                                }
                            }
                        }

                        // Insignia de la tecla Enter
                        Rectangle {
                            Layout.preferredWidth: 26
                            Layout.preferredHeight: 22
                            radius: 6
                            color: "transparent"
                            border.width: 1
                            border.color: cSurface1
                            Text {
                                anchors.centerIn: parent
                                text: "\u21B5"
                                color: cSubtext
                                font.pixelSize: 12
                            }
                        }
                    }
                }

                // Lista de resultados
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ListView {
                        id: listView
                        anchors.fill: parent
                        clip: true
                        model: appProxy
                        delegate: appDelegate
                        spacing: 4
                        currentIndex: 0
                        activeFocusOnTab: false

                        ScrollBar.vertical: ScrollBar {
                            width: 8
                            policy: ScrollBar.AsNeeded
                            background: Rectangle { color: "transparent" }
                            contentItem: Rectangle {
                                radius: 4
                                color: cSurface1
                            }
                        }
                    }

                    // Estado vacío
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 8
                        visible: appProxy.count === 0
                        z: 10

                        VectorIcon {
                            Layout.alignment: Qt.AlignHCenter
                            kind: "search"
                            color: cOverlay0
                            implicitWidth: 30
                            implicitHeight: 30
                        }
                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("No se encontraron aplicaciones")
                            color: cSubtext
                            font.pixelSize: 13
                        }
                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("Prueba con otro término")
                            color: cOverlay0
                            font.pixelSize: 11
                        }
                    }

                    // Mantiene el resaltado dentro de la lista (filtrada).
                    Connections {
                        target: appProxy
                        function onCountChanged() {
                            if (listView.count === 0)
                                return
                            if (listView.currentIndex >= listView.count)
                                listView.currentIndex = Math.max(0, listView.count - 1)
                            listView.positionViewAtIndex(listView.currentIndex, ListView.Center)
                        }
                    }
                }

                // Barra de ayuda inferior
                RowLayout {
                    Layout.fillWidth: true
                    Layout.bottomMargin: 2
                    spacing: 14

                    KeyBadge { label: "\u2191\u2193"; caption: qsTr("Mover") }
                    KeyBadge { label: "\u21B5"; caption: qsTr("Lanzar") }
                    KeyBadge { label: "Esc"; caption: qsTr("Cerrar") }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: appProxy.count + " " + qsTr("aplicaciones")
                        color: cOverlay0
                        font.pixelSize: 11
                    }
                }
            }

            // ---------------- divisoria ----------------
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: cSurface0
            }

            // ================= BARRA LATERAL (energía y control) =================
            ColumnLayout {
                Layout.preferredWidth: 104
                Layout.fillHeight: true
                Layout.leftMargin: 14
                Layout.rightMargin: 14
                Layout.topMargin: 18
                Layout.bottomMargin: 16
                spacing: 8

                Text {
                    text: qsTr("ACCIONES")
                    color: cOverlay0
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.4
                    Layout.alignment: Qt.AlignHCenter
                }

                PowerButton {
                    Layout.fillWidth: true
                    kind: "settings"
                    label: qsTr("Ajustes")
                    iconColor: cLavender
                    onTriggered: systemController.openSettings()
                }
                PowerButton {
                    Layout.fillWidth: true
                    kind: "moon"
                    label: qsTr("Suspender")
                    iconColor: cBlue
                    onTriggered: systemController.suspend()
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    radius: 1
                    color: cSurface0
                }

                Text {
                    text: qsTr("SESIÓN")
                    color: cOverlay0
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.4
                    Layout.alignment: Qt.AlignHCenter
                }

                PowerButton {
                    Layout.fillWidth: true
                    kind: "user"
                    label: qsTr("Cambiar usuario")
                    iconColor: cMauve
                    onTriggered: systemController.lockSession()
                }
                PowerButton {
                    Layout.fillWidth: true
                    kind: "reboot"
                    label: qsTr("Reiniciar")
                    iconColor: cYellow
                    hoverColor: "#3d3524"
                    onTriggered: systemController.reboot()
                }
                PowerButton {
                    Layout.fillWidth: true
                    kind: "power"
                    label: qsTr("Apagar")
                    iconColor: cRed
                    hoverColor: "#3d222b"
                    onTriggered: systemController.powerOff()
                }

                Item { Layout.fillHeight: true }

                // Reloj
                ColumnLayout {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.maximumWidth: 104
                    spacing: 2
                    Text {
                        id: clockText
                        Layout.alignment: Qt.AlignHCenter
                        text: "--:--"
                        color: cText
                        font.pixelSize: 19
                        font.weight: Font.DemiBold
                    }
                    Text {
                        id: dateText
                        Layout.fillWidth: true
                        Layout.maximumWidth: 104
                        Layout.alignment: Qt.AlignHCenter
                        text: ""
                        color: cSubtext
                        font.pixelSize: 10
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignHCenter
                    }
                    Timer {
                        interval: 1000
                        running: true
                        repeat: true
                        triggeredOnStart: true
                        onTriggered: {
                            const d = new Date()
                            clockText.text = d.toLocaleTimeString(Qt.locale(), Locale.ShortFormat)
                            dateText.text = d.toLocaleDateString(Qt.locale(), Locale.ShortFormat)
                        }
                    }
                }
            }
        }

        // ---------------- notificación tipo toast ----------------
        Rectangle {
            id: toast
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 14
            height: 34
            radius: 10
            color: cMantle
            border.width: 1
            border.color: toast.msgColor
            opacity: 0
            visible: opacity > 0

            property string msg: ""
            property color msgColor: cGreen

            Behavior on opacity { NumberAnimation { duration: 160 } }

            function show(text, bad) {
                msg = text
                msgColor = bad ? cRed : cGreen
                opacity = 1
                toastTimer.restart()
            }

            Text {
                anchors.centerIn: parent
                text: toast.msg
                color: cText
                font.pixelSize: 12
                leftPadding: 12
                rightPadding: 12
            }

            Timer {
                id: toastTimer
                interval: 2400
                onTriggered: toast.opacity = 0
            }
        }

        // ---------------- diálogo "Acerca de" (overlay modal) ----------------
        // Vive dentro del panel: al no ser una ventana independiente, el
        // lanzador nunca pierde la activación y no se cierra al abrirlo.
        Item {
            id: aboutOverlay
            anchors.fill: parent
            z: 100
            visible: opacity > 0
            opacity: 0

            Behavior on opacity { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }

            function open() {
                // Libera el foco del campo de búsqueda para no filtrar
                // resultados a espaldas del diálogo.
                searchField.focus = false
                aboutCard.scale = 0.94
                aboutCard.scale = 1
                opacity = 1
            }

            function close() {
                opacity = 0
                searchField.forceActiveFocus()
            }

            // Fondo oscurecido; al hacer clic se cierra el diálogo.
            Rectangle {
                anchors.fill: parent
                radius: 22
                color: Qt.rgba(0, 0, 0, 0.45)
                MouseArea {
                    anchors.fill: parent
                    onClicked: aboutOverlay.close()
                }
            }

            AboutDialog {
                id: aboutCard
                anchors.centerIn: parent
                appVersion: aboutAppVersion
                qtVersion: aboutQtVersion
                osName: aboutOsName
                arch: aboutArch
                appCount: appModel.count
                scale: 0.94
                Behavior on scale {
                    NumberAnimation { duration: 200; easing.type: Easing.OutBack }
                }
                onClosed: aboutOverlay.close()
            }
        }
    }

    // Comentarios del backend -> toast
    Connections {
        target: systemController
        function onCommandStarted(action) { toast.show(action, false) }
        function onCommandFailed(action, message) {
            toast.show(action + " \u2014 " + message, true)
        }
    }

    // ---------------- delegado de aplicación ----------------
    Component {
        id: appDelegate

        Rectangle {
            id: row

            required property int index
            required property string name
            required property string description
            required property string icon
            required property string exec

            property bool isCurrent: ListView.view.currentIndex === index
                                     || navMouse.containsMouse

            width: ListView.view.width
            height: 62
            radius: 12
            color: isCurrent ? cSurface1 : "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 8
                spacing: 12

                Item {
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40

                    Image {
                        id: appIcon
                        anchors.fill: parent
                        anchors.margins: 2
                        source: row.icon !== ""
                                ? "image://icons/" + encodeURIComponent(row.icon)
                                : ""
                        sourceSize.width: 40
                        sourceSize.height: 40
                        smooth: true
                        fillMode: Image.PreserveAspectFit
                        visible: row.icon !== "" && status !== Image.Error
                    }

                    VectorIcon {
                        anchors.centerIn: parent
                        kind: "app"
                        color: row.isCurrent ? cLavender : cOverlay0
                        implicitWidth: 18
                        implicitHeight: 18
                        visible: !appIcon.visible
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        Layout.fillWidth: true
                        text: row.name
                        color: cText
                        font.pixelSize: 14
                        font.weight: row.isCurrent ? Font.DemiBold : Font.Normal
                        elide: Text.ElideRight
                    }
                    Text {
                        Layout.fillWidth: true
                        visible: row.description !== ""
                        text: row.description
                        color: cSubtext
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                }

                // Botón de arranque rápido (visible en la fila actual)
                Rectangle {
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 30
                    radius: 15
                    visible: row.isCurrent
                    color: Qt.rgba(cLavender.r, cLavender.g, cLavender.b, 0.18)

                    VectorIcon {
                        anchors.centerIn: parent
                        kind: "chevron"
                        color: cLavender
                        implicitWidth: 14
                        implicitHeight: 14
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.launchAt(row.index)
                    }
                }
            }

            MouseArea {
                id: navMouse
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton
                cursorShape: Qt.PointingHandCursor
                onEntered: ListView.view.currentIndex = row.index
                onClicked: root.launchAt(row.index)
            }
        }
    }
}