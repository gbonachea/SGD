#include "SystemPanel.h"
#include "DockConfig.h"

#include <QProcess>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QHash>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>

// ─── Helpers ─────────────────────────────────────────────────────────────────

static QFrame* separador() {
    auto* f = new QFrame;
    f->setObjectName("sep");
    f->setFixedHeight(1);
    return f;
}

SystemPanel::SystemPanel(QWidget* parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedWidth(320);
    construirUI();

    setStyleSheet(R"(
QLabel { color: #eee; background: transparent; border: none; }
QLabel[section="true"] {
    color: #89b4fa; font-weight: bold; font-size: 12px;
}
QLabel#estado { color: #9aa0ab; font-size: 11px; }
QPushButton {
    color: #eee; background-color: rgba(255,255,255,18);
    border: none; border-radius: 6px; padding: 5px 9px; font-size: 12px;
}
QPushButton:hover  { background-color: rgba(255,255,255,34); }
QPushButton:pressed{ background-color: rgba(137,180,250,70); }
QPushButton:disabled { color: #666; background-color: transparent; }
QFrame#sep { background-color: rgba(255,255,255,35); border: none; }
QSlider::groove:horizontal {
    height: 4px; background: rgba(255,255,255,45); border-radius: 2px;
}
QSlider::sub-page:horizontal { background: #89b4fa; border-radius: 2px; }
QSlider::handle:horizontal {
    width: 13px; margin: -5px 0; border-radius: 7px; background: #89b4fa;
}
)");
}

// ─── UI ──────────────────────────────────────────────────────────────────────

void SystemPanel::construirUI() {
    m_contenedor = new QVBoxLayout(this);
    m_contenedor->setContentsMargins(12, 12, 12, 10);
    m_contenedor->setSpacing(8);

    // ── Red ──
    auto* hdrRed = new QHBoxLayout();
    hdrRed->setSpacing(4);
    auto* tituloRed = new QLabel("Red");
    tituloRed->setProperty("section", true);
    hdrRed->addWidget(tituloRed);
    hdrRed->addStretch(1);
    auto* btnRefresh = new QPushButton("↻");
    btnRefresh->setToolTip("Buscar redes");
    btnRefresh->setFixedSize(26, 22);
    connect(btnRefresh, &QPushButton::clicked, this, &SystemPanel::rescanearRed);
    hdrRed->addWidget(btnRefresh);
    m_contenedor->addLayout(hdrRed);

    m_zonaRed = new QWidget(this);
    m_layoutRed = new QVBoxLayout(m_zonaRed);
    m_layoutRed->setContentsMargins(0, 0, 0, 0);
    m_layoutRed->setSpacing(4);
    m_contenedor->addWidget(m_zonaRed);

    auto* btnConfigRed = new QPushButton("Configurar red…");
    connect(btnConfigRed, &QPushButton::clicked, this, &SystemPanel::abrirConfigRed);
    m_contenedor->addWidget(btnConfigRed);

    m_contenedor->addWidget(separador());

    // ── Volumen ──
    auto* tituloVol = new QLabel("Volumen");
    tituloVol->setProperty("section", true);
    m_contenedor->addWidget(tituloVol);

    auto* rowVol = new QHBoxLayout();
    rowVol->setSpacing(6);
    auto* menosV = new QPushButton("−");
    menosV->setFixedSize(28, 26);
    connect(menosV, &QPushButton::clicked, this, [this](){ cambiarVolumen(-5); });
    m_labelVol = new QLabel("—%");
    m_labelVol->setFixedWidth(42);
    m_labelVol->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_sliderVol = new QSlider(Qt::Horizontal);
    m_sliderVol->setRange(0, 100);
    connect(m_sliderVol, &QSlider::valueChanged, this, [this](int v){
        m_labelVol->setText(QString("%1%").arg(v));
    });
    connect(m_sliderVol, &QSlider::sliderReleased, this, [this](){
        setVolumen(m_sliderVol->value());
    });
    auto* masV = new QPushButton("+");
    masV->setFixedSize(28, 26);
    connect(masV, &QPushButton::clicked, this, [this](){ cambiarVolumen(5); });
    rowVol->addWidget(menosV);
    rowVol->addWidget(m_sliderVol, 1);
    rowVol->addWidget(m_labelVol);
    rowVol->addWidget(masV);
    m_contenedor->addLayout(rowVol);

    m_btnMute = new QPushButton("Silenciar");
    connect(m_btnMute, &QPushButton::clicked, this, &SystemPanel::alternarMute);
    m_contenedor->addWidget(m_btnMute);

    m_contenedor->addWidget(separador());

    // ── Brillo ──
    auto* tituloBrillo = new QLabel("Brillo");
    tituloBrillo->setProperty("section", true);
    m_contenedor->addWidget(tituloBrillo);

    auto* rowBri = new QHBoxLayout();
    rowBri->setSpacing(6);
    auto* menosB = new QPushButton("−");
    menosB->setFixedSize(28, 26);
    connect(menosB, &QPushButton::clicked, this, [this](){ cambiarBrillo(-5); });
    m_labelBrillo = new QLabel("—%");
    m_labelBrillo->setFixedWidth(42);
    m_labelBrillo->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_sliderBrillo = new QSlider(Qt::Horizontal);
    m_sliderBrillo->setRange(0, 100);
    connect(m_sliderBrillo, &QSlider::valueChanged, this, [this](int v){
        m_labelBrillo->setText(QString("%1%").arg(v));
    });
    connect(m_sliderBrillo, &QSlider::sliderReleased, this, [this](){
        setBrillo(m_sliderBrillo->value());
    });
    auto* masB = new QPushButton("+");
    masB->setFixedSize(28, 26);
    connect(masB, &QPushButton::clicked, this, [this](){ cambiarBrillo(5); });
    rowBri->addWidget(menosB);
    rowBri->addWidget(m_sliderBrillo, 1);
    rowBri->addWidget(m_labelBrillo);
    rowBri->addWidget(masB);
    m_contenedor->addLayout(rowBri);

    m_contenedor->addWidget(separador());

    m_estado = new QLabel("");
    m_estado->setObjectName("estado");
    m_estado->setWordWrap(true);
    m_contenedor->addWidget(m_estado);
}

// ─── Pintado ─────────────────────────────────────────────────────────────────
// Fondo del panel: relleno opaco con esquinas redondeadas y borde sutil,
// pintado a mano (más fiable que la hoja de estilos sobre ventana translúcida)
// para que redes, volumen y brillo se vean con nitidez sobre cualquier fondo.

void SystemPanel::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath path;
    path.addRoundedRect(rect().adjusted(0, 0, -1, -1), 12, 12);

    // Fondo totalmente opaco
    p.fillPath(path, QColor(20, 20, 26));

    // Borde sutil para despegar el panel del escritorio
    p.setPen(QPen(QColor(255, 255, 255, 42), 1));
    p.drawPath(path);
}

// ─── Apertura ────────────────────────────────────────────────────────────────

void SystemPanel::abrirSobre(const QRect& ancla, DockPosition pos) {
    // Primero poblar los datos para que el layout tenga su tamaño final
    // y el anclaje se calcule con la altura real del panel.
    actualizarVolumen();
    actualizarBrillo();
    actualizarRed();

    adjustSize();
    QSize s = size();
    const int gap = 8;
    QPoint p;
    switch (pos) {
    case DockPosition::Abajo:     p = QPoint(ancla.right() - s.width(),  ancla.top()     - s.height() - gap); break;
    case DockPosition::Arriba:    p = QPoint(ancla.right() - s.width(),  ancla.bottom()  + gap);               break;
    case DockPosition::Izquierda: p = QPoint(ancla.right()  + gap,        ancla.bottom()  - s.height());       break;
    case DockPosition::Derecha:   p = QPoint(ancla.left()   - s.width() - gap, ancla.bottom() - s.height());  break;
    }

    if (QScreen* scr = QGuiApplication::screenAt(ancla.center())) {
        QRect sg = scr->availableGeometry();
        p.setX(qBound(sg.left(),  p.x(), sg.right()  - s.width()  + 1));
        p.setY(qBound(sg.top(),   p.y(), sg.bottom() - s.height() + 1));
    }

    move(p);
    show();
    raise();
}

QString SystemPanel::ejecutar(const QString& cmd, int timeoutMs) const {
    QProcess p;
    p.start(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), cmd});
    if (!p.waitForStarted(800)) return {};
    if (!p.waitForFinished(timeoutMs)) { p.kill(); p.waitForFinished(300); return {}; }
    return QString::fromLocal8Bit(p.readAllStandardOutput());
}

void SystemPanel::setEstado(const QString& txt) {
    if (m_estado) m_estado->setText(txt);
}

// ─── Red ─────────────────────────────────────────────────────────────────────

void SystemPanel::actualizarRed() {
    if (QStandardPaths::findExecutable(QStringLiteral("nmcli")).isEmpty()) {
        setEstado(QStringLiteral("NetworkManager no disponible (falta nmcli)"));
        poblarRed(QString());
        return;
    }
    setEstado(QString());
    poblarRed(ejecutar(QStringLiteral("nmcli -t -f SSID,SIGNAL dev wifi list --rescan no"), 2500));
}

void SystemPanel::rescanearRed() {
    if (QStandardPaths::findExecutable(QStringLiteral("nmcli")).isEmpty()) return;
    setEstado(QStringLiteral("Escaneando redes…"));
    QProcess* p = new QProcess(this);
    connect(p, &QProcess::finished, this, [this, p](int, QProcess::ExitStatus){
        QString out = QString::fromLocal8Bit(p->readAllStandardOutput());
        p->deleteLater();
        poblarRed(out);
        setEstado(QString());
    });
    p->start(QStringLiteral("nmcli"),
             {QStringLiteral("-t"), QStringLiteral("-f"), QStringLiteral("SSID,SIGNAL"),
              QStringLiteral("dev"), QStringLiteral("wifi"), QStringLiteral("list"),
              QStringLiteral("--rescan"), QStringLiteral("yes")});
}

void SystemPanel::poblarRed(const QString& out) {
    // Limpiar la lista anterior
    while (QLayoutItem* child = m_layoutRed->takeAt(0)) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    struct Red { QString ssid; int signal; };
    QHash<QString, int> mejor;
    for (const QString& linea : out.split('\n')) {
        QString l = linea.trimmed();
        if (l.isEmpty()) continue;
        int i = l.lastIndexOf(':');
        if (i <= 0) continue;                       // sin SSID (red oculta)
        QString ssid = l.left(i);
        bool ok = false;
        int sig = l.mid(i + 1).toInt(&ok);
        if (!ok) continue;
        if (!mejor.contains(ssid) || mejor.value(ssid) < sig) mejor.insert(ssid, sig);
    }

    QList<Red> redes;
    for (auto it = mejor.cbegin(); it != mejor.cend(); ++it)
        redes.append({it.key(), it.value()});
    std::sort(redes.begin(), redes.end(),
              [](const Red& a, const Red& b){ return a.signal > b.signal; });

    if (redes.isEmpty()) {
        auto* l = new QLabel(QStringLiteral("Sin redes detectadas"));
        l->setStyleSheet(QStringLiteral("color: #888; padding: 4px;"));
        m_layoutRed->addWidget(l);
        return;
    }

    for (const Red& r : redes) {
        QString ssid = r.ssid;
        auto* b = new QPushButton(QStringLiteral("%1   %2%").arg(r.ssid).arg(r.signal));
        b->setStyleSheet(QStringLiteral(
            "text-align:left; padding:5px 8px; font-size:12px; color:#eee;"
            "background-color:rgba(255,255,255,14); border:none; border-radius:6px;"));
        connect(b, &QPushButton::clicked, this, [this, ssid](){ conectarWifi(ssid); });
        m_layoutRed->addWidget(b);
    }
}

void SystemPanel::conectarWifi(const QString& ssid) {
    setEstado(QStringLiteral("Conectando a «%1»…").arg(ssid));
    QProcess* p = new QProcess(this);
    connect(p, &QProcess::finished, this, [this, p, ssid](int, QProcess::ExitStatus){
        QString out = QString::fromLocal8Bit(p->readAllStandardOutput()
                                             + p->readAllStandardError()).trimmed();
        p->deleteLater();
        if (p->exitStatus() == QProcess::NormalExit && p->exitCode() == 0) {
            setEstado(QStringLiteral("Conectado a «%1»").arg(ssid));
        } else {
            QString detalle = out.split('\n').last();
            setEstado(QStringLiteral("Error en «%1»: %2")
                          .arg(ssid, detalle.isEmpty() ? QStringLiteral("sin detalle") : detalle));
        }
    });
    p->start(QStringLiteral("nmcli"), {QStringLiteral("device"), QStringLiteral("wifi"),
                                       QStringLiteral("connect"), ssid});
}

void SystemPanel::abrirConfigRed() {
    struct Candidato { QString exe; QStringList args; };
    static const QList<Candidato> candidatos = {
        {QStringLiteral("nm-connection-editor"), {}},
        {QStringLiteral("gnome-control-center"), {QStringLiteral("network")}},
        {QStringLiteral("xfce4-settings-manager"), {}},
        {QStringLiteral("systemsettings"), {}},
    };
    for (const auto& c : candidatos) {
        QString exe = QStandardPaths::findExecutable(c.exe);
        if (!exe.isEmpty()) {
            QProcess::startDetached(exe, c.args);
            return;
        }
    }
    setEstado(QStringLiteral("No se encontró un configurador de red."));
}

// ─── Volumen ─────────────────────────────────────────────────────────────────

void SystemPanel::actualizarVolumen() {
    m_volPactl = !QStandardPaths::findExecutable(QStringLiteral("pactl")).isEmpty();

    bool muted = false;
    int  v = -1;
    if (m_volPactl) {
        QString out = ejecutar(QStringLiteral("pactl get-sink-volume @DEFAULT_SINK@"));
        QRegularExpression re(QStringLiteral("(\\d+)%"));
        QRegularExpressionMatch m = re.match(out);
        if (m.hasMatch()) v = m.captured(1).toInt();
        muted = ejecutar(QStringLiteral("pactl get-sink-mute @DEFAULT_SINK@")).contains(QStringLiteral("yes"));
    } else {
        QString out = ejecutar(QStringLiteral("amixer -M get Master"));
        QRegularExpression re(QStringLiteral("\\[(\\d+)%\\]"));
        QRegularExpressionMatch m = re.match(out);
        if (m.hasMatch()) v = m.captured(1).toInt();
        muted = out.contains(QStringLiteral("[off]"));
    }

    if (v >= 0) {
        m_sliderVol->setValue(v);
        m_labelVol->setText(QStringLiteral("%1%").arg(v));
    }
    bool ok = (v >= 0);
    m_sliderVol->setEnabled(ok);
    m_btnMute->setEnabled(ok);
    m_btnMute->setText(muted ? QStringLiteral("Activar sonido")
                             : QStringLiteral("Silenciar"));
}

void SystemPanel::setVolumen(int v) {
    v = qBound(0, v, 100);
    if (m_volPactl) {
        QProcess::startDetached(QStringLiteral("pactl"),
                                {QStringLiteral("set-sink-volume"),
                                 QStringLiteral("@DEFAULT_SINK@"),
                                 QStringLiteral("%1%").arg(v)});
    } else {
        QProcess::startDetached(QStringLiteral("amixer"),
                                {QStringLiteral("-q"), QStringLiteral("-M"), QStringLiteral("set"),
                                 QStringLiteral("Master"), QStringLiteral("%1%").arg(v)});
    }
}

void SystemPanel::cambiarVolumen(int delta) {
    if (m_volPactl) {
        QProcess::startDetached(QStringLiteral("pactl"),
                                {QStringLiteral("set-sink-volume"),
                                 QStringLiteral("@DEFAULT_SINK@"),
                                 QStringLiteral("%1%2").arg(qAbs(delta)).arg(delta > 0 ? "+" : "-")});
    } else {
        QProcess::startDetached(QStringLiteral("amixer"),
                                {QStringLiteral("-q"), QStringLiteral("-M"), QStringLiteral("set"),
                                 QStringLiteral("Master"),
                                 QStringLiteral("%1%2").arg(qAbs(delta)).arg(delta > 0 ? "+" : "-")});
    }
    QTimer::singleShot(250, this, [this]{ actualizarVolumen(); });
}

void SystemPanel::alternarMute() {
    if (m_volPactl) {
        QProcess::startDetached(QStringLiteral("pactl"),
                                {QStringLiteral("set-sink-mute"),
                                 QStringLiteral("@DEFAULT_SINK@"),
                                 QStringLiteral("toggle")});
    } else {
        QProcess::startDetached(QStringLiteral("amixer"),
                                {QStringLiteral("-q"), QStringLiteral("set"),
                                 QStringLiteral("Master"), QStringLiteral("toggle")});
    }
    QTimer::singleShot(250, this, [this]{ actualizarVolumen(); });
}

// ─── Brillo ──────────────────────────────────────────────────────────────────

QString SystemPanel::rutaBacklight() const {
    QDir d(QStringLiteral("/sys/class/backlight"));
    const QStringList entradas = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    return entradas.isEmpty() ? QString() : d.filePath(entradas.first());
}

void SystemPanel::actualizarBrillo() {
    m_brilloCtl = !QStandardPaths::findExecutable(QStringLiteral("brightnessctl")).isEmpty();

    if (m_brilloCtl) {
        bool ok1 = false, ok2 = false;
        int cur = ejecutar(QStringLiteral("brightnessctl get")).trimmed().toInt(&ok1);
        int max = ejecutar(QStringLiteral("brightnessctl max")).trimmed().toInt(&ok2);
        if (ok1 && ok2 && max > 0)
            m_sliderBrillo->setValue(qBound(0, (int)(cur * 100.0 / max + 0.5), 100));
        m_sliderBrillo->setEnabled(ok1 && ok2 && max > 0);
        return;
    }

    m_backlightDir = rutaBacklight();
    m_brilloSysfs  = !m_backlightDir.isEmpty();
    if (m_brilloSysfs) {
        QFile fMax(m_backlightDir + QStringLiteral("/max_brightness"));
        QFile fCur(m_backlightDir + QStringLiteral("/brightness"));
        if (fMax.open(QIODevice::ReadOnly) && fCur.open(QIODevice::ReadOnly)) {
            bool ok1 = false, ok2 = false;
            int max = fMax.readAll().trimmed().toInt(&ok1);
            int cur = fCur.readAll().trimmed().toInt(&ok2);
            if (ok1 && ok2 && max > 0)
                m_sliderBrillo->setValue(qBound(0, (int)(cur * 100.0 / max + 0.5), 100));
            m_sliderBrillo->setEnabled(ok1 && ok2 && max > 0);
        } else {
            m_sliderBrillo->setEnabled(false);
        }
    } else {
        m_sliderBrillo->setEnabled(false);
        m_labelBrillo->setText(QStringLiteral("—"));
        setEstado(QStringLiteral("Brillo no disponible (instala brightnessctl)."));
    }
}

void SystemPanel::setBrillo(int v) {
    v = qBound(0, v, 100);
    if (m_brilloCtl) {
        QProcess::startDetached(QStringLiteral("brightnessctl"),
                                {QStringLiteral("-q"), QStringLiteral("set"),
                                 QStringLiteral("%1%").arg(v)});
        return;
    }
    if (m_brilloSysfs && !m_backlightDir.isEmpty()) {
        QFile fMax(m_backlightDir + QStringLiteral("/max_brightness"));
        QFile fBri(m_backlightDir + QStringLiteral("/brightness"));
        if (fMax.open(QIODevice::ReadOnly) && fBri.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            int max = fMax.readAll().trimmed().toInt();
            if (max > 0) {
                fBri.write(QByteArray::number(max * v / 100));
                fBri.close();
            }
        } else {
            setEstado(QStringLiteral("Brillo: permiso denegado (requiere root)."));
            QTimer::singleShot(200, this, [this]{ actualizarBrillo(); });
        }
    }
}

void SystemPanel::cambiarBrillo(int delta) {
    setBrillo(m_sliderBrillo->value() + delta);
    QTimer::singleShot(250, this, [this]{ actualizarBrillo(); });
}