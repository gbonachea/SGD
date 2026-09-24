import QtQuick

/**
 * Conjunto ligero de iconos vectoriales dibujados con Canvas.
 * No requiere fuentes emoji, por lo que los iconos se muestran idénticos en
 * cualquier escritorio Linux.
 *
 * Tipos admitidos:
 *   power | moon | user | reboot | settings | search
 *   sparkle | play | chevron | close | app
 */
Canvas {
    id: canvas

    property string kind: "power"
    property color color: "#cdd6f4"
    property real strokeWidth: 2

    implicitWidth: 22
    implicitHeight: 22

    onKindChanged: requestPaint()
    onColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d")
        if (ctx == null || width === 0 || height === 0)
            return

        ctx.clearRect(0, 0, width, height)
        ctx.globalCompositeOperation = "source-over"
        ctx.setTransform(1, 0, 0, 1, 0, 0)
        ctx.save()
        ctx.scale(width / 24, height / 24)
        ctx.lineCap = "round"
        ctx.lineJoin = "round"
        ctx.lineWidth = strokeWidth
        ctx.strokeStyle = color
        ctx.fillStyle = color

        switch (kind) {
        case "power":    paintPower(ctx); break
        case "moon":     paintMoon(ctx); break
        case "user":     paintUser(ctx); break
        case "reboot":   paintReboot(ctx); break
        case "settings": paintSettings(ctx); break
        case "search":   paintSearch(ctx); break
        case "sparkle":  paintSparkle(ctx); break
        case "play":     paintPlay(ctx); break
        case "chevron":  paintChevron(ctx); break
        case "close":    paintClose(ctx); break
        case "app":      paintApp(ctx); break
        }
        ctx.restore()
    }

    function paintPower(ctx) {
        ctx.beginPath()
        ctx.arc(12, 13.5, 6, 1.5 * Math.PI + 0.45, 1.5 * Math.PI - 0.45, false)
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(12, 7.5)
        ctx.lineTo(12, 2.5)
        ctx.stroke()
    }

    function paintMoon(ctx) {
        ctx.beginPath()
        ctx.arc(14, 12, 6.6, 0, 2 * Math.PI)
        ctx.fill()
        ctx.globalCompositeOperation = "destination-out"
        ctx.beginPath()
        ctx.arc(10.4, 9.6, 5.4, 0, 2 * Math.PI)
        ctx.fill()
        ctx.globalCompositeOperation = "source-over"
    }

    function paintUser(ctx) {
        // cabeza
        ctx.beginPath()
        ctx.arc(12, 9, 3.4, 0, 2 * Math.PI)
        ctx.fill()
        // hombros
        ctx.beginPath()
        ctx.arc(12, 21.5, 8, Math.PI, 2 * Math.PI, false)
        ctx.closePath()
        ctx.fill()
    }

    function paintReboot(ctx) {
        ctx.beginPath()
        ctx.arc(12, 12, 6.2, -0.35 * Math.PI, 1.45 * Math.PI, false)
        ctx.stroke()

        const a = -0.35 * Math.PI
        const tip = Qt.point(12 + 6.2 * Math.cos(a), 12 + 6.2 * Math.sin(a))
        const dirX = -Math.sin(a)
        const dirY = Math.cos(a)
        const perpX = dirY
        const perpY = -dirX
        const len = 3.4
        const wid = 2.4
        const backX = tip.x - dirX * len
        const backY = tip.y - dirY * len

        ctx.beginPath()
        ctx.moveTo(tip.x, tip.y)
        ctx.lineTo(backX + perpX * wid / 2, backY + perpY * wid / 2)
        ctx.lineTo(backX - perpX * wid / 2, backY - perpY * wid / 2)
        ctx.closePath()
        ctx.fill()
    }

    function paintSettings(ctx) {
        // centro
        ctx.beginPath()
        ctx.arc(12, 12, 3.4, 0, 2 * Math.PI)
        ctx.fill()
        // dientes de la estrella
        for (let i = 0; i < 6; i++) {
            ctx.save()
            ctx.translate(12, 12)
            ctx.rotate(i * Math.PI / 3)
            ctx.fillRect(10.7, 5.8, 2.6, 2.6)
            ctx.restore()
        }
        // agujero central
        ctx.globalCompositeOperation = "destination-out"
        ctx.beginPath()
        ctx.arc(12, 12, 1.3, 0, 2 * Math.PI)
        ctx.fill()
        ctx.globalCompositeOperation = "source-over"
    }

    function paintSearch(ctx) {
        ctx.beginPath()
        ctx.arc(9.2, 9.2, 5.4, 0, 2 * Math.PI)
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(13.1, 13.1)
        ctx.lineTo(19.5, 19.5)
        ctx.stroke()
    }

    function paintSparkle(ctx) {
        ctx.beginPath()
        for (let i = 0; i < 8; i++) {
            const ang = i * Math.PI / 4 - Math.PI / 2
            const r = (i % 2 === 0) ? 10 : 3.6
            const x = 12 + r * Math.cos(ang)
            const y = 12 + r * Math.sin(ang)
            if (i === 0)
                ctx.moveTo(x, y)
            else
                ctx.lineTo(x, y)
        }
        ctx.closePath()
        ctx.fill()
    }

    function paintPlay(ctx) {
        ctx.beginPath()
        ctx.moveTo(9, 6.5)
        ctx.lineTo(18, 12)
        ctx.lineTo(9, 17.5)
        ctx.closePath()
        ctx.fill()
    }

    function paintChevron(ctx) {
        ctx.beginPath()
        ctx.moveTo(9.2, 8)
        ctx.lineTo(14, 12)
        ctx.lineTo(9.2, 16)
        ctx.stroke()
    }

    function paintClose(ctx) {
        ctx.beginPath()
        ctx.moveTo(6, 6)
        ctx.lineTo(18, 18)
        ctx.moveTo(18, 6)
        ctx.lineTo(6, 18)
        ctx.stroke()
    }

    function paintApp(ctx) {
        ctx.beginPath()
        ctx.arc(8.6, 8.6, 1.9, 0, 2 * Math.PI)
        ctx.arc(15.4, 8.6, 1.9, 0, 2 * Math.PI)
        ctx.arc(8.6, 15.4, 1.9, 0, 2 * Math.PI)
        ctx.arc(15.4, 15.4, 1.9, 0, 2 * Math.PI)
        ctx.fill()
    }
}