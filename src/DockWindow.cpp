#include "DockWindow.h"
#include "DockItem.h"
#include "ApplicationScanner.h"
#include "SettingsDialog.h"
#include "SystemPanel.h"
#include "x11util.h"

#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QApplication>
#include <QMouseEvent>
#include <QMenu>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QDebug>
#include <QGuiApplication>
#include <QTimer>
#include <QIcon>
#include <QDateTime>
#include <cmath>

// ─── Constructor ────────────────────────────────────────────────────────────

DockWindow::DockWindow(QWidget* parent) : QWidget(parent) {
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint |
                   Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setMouseTracking(true);

    QString cfgPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                      + "/sgd/sgd.conf";
    QDir().mkpath(QFileInfo(cfgPath).dir().absolutePath());
    m_settings = new QSettings(cfgPath, QSettings::IniFormat, this);

    m_scanner = new ApplicationScanner(this);

    m_timerWM = new QTimer(this);
    m_timerWM->setInterval(800);
    connect(m_timerWM, &QTimer::timeout, this, &DockWindow::actualizarVentanas);

    m_timerAutoOcultar = new QTimer(this);
    m_timerAutoOcultar->setSingleShot(true);
    m_timerAutoOcultar->setInterval(600);
    connect(m_timerAutoOcultar, &QTimer::timeout, this, &DockWindow::ocultarDock);

    m_timerResetMag = new QTimer(this);
    m_timerResetMag->setSingleShot(true);
    m_timerResetMag->setInterval(80);
    connect(m_timerResetMag, &QTimer::timeout, this, &DockWindow::resetMagnificacion);

    m_timerPantalla = new QTimer(this);
    m_timerPantalla->setInterval(2000);
    connect(m_timerPantalla, &QTimer::timeout, this, &DockWindow::posicionarDock);
    m_timerPantalla->start();

    m_animSlide = new QPropertyAnimation(this, "posOffset", this);
    m_animSlide->setDuration(220);
    m_animSlide->setEasingCurve(QEasingCurve::InOutCubic);

    cargarConfig();

    // Botón fijo del lanzador de aplicaciones (Stellar Launcher): siempre
    // primero en el dock.
    auto* lanzador = new DockItem(DockItem::Tipo::Lanzador, this, this);
    lanzador->setNombre(QStringLiteral("Lanzador"));
    lanzador->setIcono(QIcon(QStringLiteral(":/launcher/stellar-lanzador.png")));
    conectarItem(lanzador);
    lanzador->show();
    m_items.append(lanzador);

    cargarAppsAncladas();

    // Bloque fijo del final del dock: separador + reloj.
    auto* sep = new DockItem(DockItem::Tipo::Separador, this, this);
    conectarItem(sep);
    sep->show();
    m_items.append(sep);

    auto* reloj = new DockItem(DockItem::Tipo::Reloj, this, this);
    conectarItem(reloj);
    reloj->show();
    m_items.append(reloj);

    reconstruirDock();
    m_timerWM->start();
}

DockWindow::~DockWindow() {
    guardarConfig();
    guardarAppsAncladas();
    x11_clear_strut((unsigned long)winId());
}

// ─── Config ─────────────────────────────────────────────────────────────────

void DockWindow::cargarConfig() {
    m_settings->beginGroup("Dock");
    m_cfg.posicion           = (DockPosition)m_settings->value("posicion", 0).toInt();
    m_cfg.comportamiento     = (DockComportamiento)m_settings->value("comportamiento", 0).toInt();
    m_cfg.tamanoIcono        = m_settings->value("tamanoIcono", 32).toInt();
    m_cfg.magnificacion      = m_settings->value("magnificacion", true).toBool();
    m_cfg.multiplicadorMag   = m_settings->value("multiplicadorMag", 160).toInt();
    m_cfg.fondoTranslucido   = m_settings->value("fondoTranslucido", true).toBool();
    m_cfg.opacidadFondo      = m_settings->value("opacidadFondo", 0.80).toDouble();
    m_cfg.mostrarIndicadores = m_settings->value("mostrarIndicadores", true).toBool();
    m_cfg.mostrarEtiquetas   = m_settings->value("mostrarEtiquetas", true).toBool();
    m_cfg.rebotarAlAbrir     = m_settings->value("rebotarAlAbrir", true).toBool();
    m_cfg.indiceMonitor      = m_settings->value("indiceMonitor", 0).toInt();
    m_cfg.radioEsquina       = m_settings->value("radioEsquina", 16).toInt();
    m_cfg.espaciadoItems     = m_settings->value("espaciadoItems", 6).toInt();
    m_settings->endGroup();
}

void DockWindow::guardarConfig() {
    m_settings->beginGroup("Dock");
    m_settings->setValue("posicion",           (int)m_cfg.posicion);
    m_settings->setValue("comportamiento",     (int)m_cfg.comportamiento);
    m_settings->setValue("tamanoIcono",        m_cfg.tamanoIcono);
    m_settings->setValue("magnificacion",      m_cfg.magnificacion);
    m_settings->setValue("multiplicadorMag",   m_cfg.multiplicadorMag);
    m_settings->setValue("fondoTranslucido",   m_cfg.fondoTranslucido);
    m_settings->setValue("opacidadFondo",      m_cfg.opacidadFondo);
    m_settings->setValue("mostrarIndicadores", m_cfg.mostrarIndicadores);
    m_settings->setValue("mostrarEtiquetas",   m_cfg.mostrarEtiquetas);
    m_settings->setValue("rebotarAlAbrir",     m_cfg.rebotarAlAbrir);
    m_settings->setValue("indiceMonitor",      m_cfg.indiceMonitor);
    m_settings->setValue("radioEsquina",       m_cfg.radioEsquina);
    m_settings->setValue("espaciadoItems",     m_cfg.espaciadoItems);
    m_settings->endGroup();
    m_settings->sync();
}

void DockWindow::cargarAppsAncladas() {
    QStringList defaults;
    QStringList candidatos = {
        "/usr/share/applications/org.gnome.Nautilus.desktop",
        "/usr/share/applications/nautilus.desktop",
        "/usr/share/applications/nemo.desktop",
        "/usr/share/applications/thunar.desktop",
        "/usr/share/applications/firefox.desktop",
        "/usr/share/applications/firefox-esr.desktop",
        "/usr/share/applications/chromium-browser.desktop",
        "/usr/share/applications/google-chrome.desktop",
        "/usr/share/applications/org.gnome.Terminal.desktop",
        "/usr/share/applications/gnome-terminal.desktop",
        "/usr/share/applications/konsole.desktop",
        "/usr/share/applications/org.gnome.gedit.desktop",
        "/usr/share/applications/gedit.desktop",
        "/usr/share/applications/org.gnome.Settings.desktop",
        "/usr/share/applications/gnome-control-center.desktop",
    };
    for (const QString& c : candidatos) {
        if (QFile::exists(c)) defaults << c;
        if (defaults.size() >= 5) break;
    }

    m_settings->beginGroup("AppsAncladas");
    QStringList ancladas = m_settings->value("lista", defaults).toStringList();
    m_settings->endGroup();

    for (const QString& df : ancladas) {
        InfoApp info = m_scanner->buscarPorDesktop(df);
        if (info.nombre.isEmpty()) {
            if (!QFile::exists(df)) continue;
            info.archivoDesktop = df;
            info.nombre = QFileInfo(df).baseName();
            info.icono  = QIcon::fromTheme("application-x-executable");
        }
        info.estaAnclada = true;
        auto* item = new DockItem(info, this, this);
        conectarItem(item);
        item->show();
        insertarItem(item);
    }
}

void DockWindow::guardarAppsAncladas() {
    QStringList lista;
    for (DockItem* it : m_items)
        if (it->tipo() == DockItem::Tipo::Aplicacion && it->estaAnclada())
            lista << it->infoApp().archivoDesktop;
    m_settings->beginGroup("AppsAncladas");
    m_settings->setValue("lista", lista);
    m_settings->endGroup();
    m_settings->sync();
}

void DockWindow::aplicarConfig(const DockConfig& cfg) {
    m_cfg = cfg;
    guardarConfig();
    for (DockItem* it : m_items) { it->setTamanoMagnificado(0); it->update(); }
    x11_clear_strut((unsigned long)winId());
    layoutItems();
    posicionarDock();
    establecerEstrut();
    update();
}

// ─── Items ──────────────────────────────────────────────────────────────────

void DockWindow::conectarItem(DockItem* item) {
    connect(item, &DockItem::clicIzquierdo, this, [this, item](){ alHacerClicItem(item); });
    connect(item, &DockItem::clicDerecho,   this, [this, item](QPoint pos){ alHacerClicDerechoItem(item, pos); });
    connect(item, &DockItem::clicMedio,     this, [this, item](){
        if (item->tipo() == DockItem::Tipo::Lanzador) abrirLanzador();
        else if (item->tipo() == DockItem::Tipo::Reloj) abrirPanelSistema(item);
        else item->lanzar();
    });
}

void DockWindow::reconstruirDock() {
    for (DockItem* it : m_items) { disconnect(it, nullptr, this, nullptr); conectarItem(it); }
    layoutItems();
    posicionarDock();
    establecerEstrut();
}

void DockWindow::anclarApp(const QString& df) {
    for (DockItem* it : m_items)
        if (it->infoApp().archivoDesktop == df) { it->setAnclada(true); return; }
    InfoApp info = m_scanner->buscarPorDesktop(df);
    if (info.nombre.isEmpty()) return;
    info.estaAnclada = true;
    auto* item = new DockItem(info, this, this);
    conectarItem(item);
    item->show();
    insertarItem(item);
    layoutItems();
    posicionarDock();
    guardarAppsAncladas();
}

void DockWindow::desanclarApp(const QString& df) {
    for (int i = 0; i < m_items.size(); i++) {
        DockItem* it = m_items[i];
        if (it->infoApp().archivoDesktop == df) {
            if (it->estaEjecutando()) { it->setAnclada(false); }
            else { m_items.removeAt(i); it->deleteLater(); layoutItems(); posicionarDock(); }
            guardarAppsAncladas();
            return;
        }
    }
}

void DockWindow::insertarItem(DockItem* item) {
    // Inserta un item de aplicación antes del bloque fijo del final
    // (separador + reloj) para que el reloj quede siempre al final del dock.
    int idx = m_items.size();
    for (int i = m_items.size() - 1; i >= 0; i--) {
        DockItem::Tipo t = m_items[i]->tipo();
        if (t == DockItem::Tipo::Reloj || t == DockItem::Tipo::Separador) idx = i;
        else break;
    }
    m_items.insert(idx, item);
}

// ─── Layout ─────────────────────────────────────────────────────────────────

void DockWindow::layoutItems() {
    bool horizontal = (m_cfg.posicion == DockPosition::Abajo ||
                       m_cfg.posicion == DockPosition::Arriba);
    int esp    = m_cfg.espaciadoItems;
    int pad    = m_cfg.padding;
    int grosor = m_cfg.tamanoIcono + 20 + pad * 2;

    if (horizontal) {
        int totalW = pad;
        for (DockItem* it : m_items) totalW += it->tamano().width() + esp;
        if (!m_items.isEmpty()) totalW -= esp;
        totalW += pad;
        resize(totalW, grosor);
        int x = pad;
        for (DockItem* it : m_items) {
            int w = it->tamano().width();
            it->setGeometry(x, 0, w, grosor);
            it->show();
            x += w + esp;
        }
    } else {
        int totalH = pad;
        for (DockItem* it : m_items) totalH += it->tamano().height() + esp;
        if (!m_items.isEmpty()) totalH -= esp;
        totalH += pad;
        resize(grosor, totalH);
        int y = pad;
        for (DockItem* it : m_items) {
            int h = it->tamano().height();
            it->setGeometry(0, y, grosor, h);
            it->show();
            y += h + esp;
        }
    }
}

QScreen* DockWindow::pantallaObjetivo() const {
    QList<QScreen*> pantallas = QApplication::screens();
    int idx = qBound(0, m_cfg.indiceMonitor, pantallas.size()-1);
    return pantallas[idx];
}

QRect DockWindow::geometriaDock() const {
    QScreen* s  = pantallaObjetivo();
    QRect    sg = s->geometry();
    int grosor  = m_cfg.tamanoIcono + 20 + m_cfg.padding * 2;
    int mg      = m_cfg.margen;

    switch (m_cfg.posicion) {
    case DockPosition::Abajo:
        return QRect(sg.x() + (sg.width()  - width())  / 2,
                     sg.y() + sg.height() - grosor - mg,
                     width(), grosor);
    case DockPosition::Arriba:
        return QRect(sg.x() + (sg.width()  - width())  / 2,
                     sg.y() + mg, width(), grosor);
    case DockPosition::Izquierda:
        return QRect(sg.x() + mg,
                     sg.y() + (sg.height() - height()) / 2,
                     grosor, height());
    case DockPosition::Derecha:
        return QRect(sg.x() + sg.width() - grosor - mg,
                     sg.y() + (sg.height() - height()) / 2,
                     grosor, height());
    }
    return {};
}

void DockWindow::posicionarDock() {
    layoutItems();
    QRect r = geometriaDock();
    move(r.topLeft());
}

void DockWindow::setPosOffset(int v) {
    m_posOffset = v;
    QRect base = geometriaDock();
    switch (m_cfg.posicion) {
    case DockPosition::Abajo:     move(base.x(), base.y() + v); break;
    case DockPosition::Arriba:    move(base.x(), base.y() - v); break;
    case DockPosition::Izquierda: move(base.x() - v, base.y()); break;
    case DockPosition::Derecha:   move(base.x() + v, base.y()); break;
    }
}

// ─── Paint ──────────────────────────────────────────────────────────────────

void DockWindow::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRect bg = rect().adjusted(2, 2, -2, -2);
    int r = m_cfg.radioEsquina;
    QPainterPath path;
    path.addRoundedRect(bg, r, r);
    QColor bgColor(30, 30, 30, (int)(m_cfg.opacidadFondo * 255));
    p.fillPath(path, bgColor);
    p.setPen(QPen(QColor(255,255,255,45), 1.2));
    p.drawPath(path);
    QPainterPath inner;
    inner.addRoundedRect(bg.adjusted(1,1,-1,-1), r-1, r-1);
    p.setPen(QPen(QColor(255,255,255,18), 1));
    p.drawPath(inner);
}

// ─── WM ─────────────────────────────────────────────────────────────────────

void DockWindow::showEvent(QShowEvent* e) {
    QWidget::showEvent(e);
    QTimer::singleShot(150, this, [this](){
        configurarWM();
        establecerEstrut();
        posicionarDock();
    });
}

void DockWindow::configurarWM() {
    x11_set_dock_type((unsigned long)winId());
}

void DockWindow::establecerEstrut() {
    unsigned long wid = (unsigned long)winId();
    if (!wid) return;
    QScreen* s  = pantallaObjetivo();
    QRect    sg = s->geometry();
    int grosor  = m_cfg.tamanoIcono + 20 + m_cfg.padding * 2 + m_cfg.margen;
    x11_set_strut(wid, sg.x(), sg.y(), sg.width(), sg.height(),
                  (int)m_cfg.posicion, grosor);
}

void DockWindow::limpiarEstrut() {
    x11_clear_strut((unsigned long)winId());
}

// ─── Window Tracking ────────────────────────────────────────────────────────

void DockWindow::actualizarVentanas() {
    static const int MAX_WINS = 256;
    X11WindowInfo wins[MAX_WINS];
    int n = x11_get_client_list(wins, MAX_WINS);

    QSet<quint64> actuales;
    bool layoutChanged = false;

    for (int i = 0; i < n; i++) {
        quint64 wid = (quint64)wins[i].wid;
        if (wid == (quint64)winId()) continue;
        actuales.insert(wid);

        QString wmClass = QString::fromLocal8Bit(wins[i].wmClass).toLower();
        QString wmName  = QString::fromLocal8Bit(wins[i].wmName).toLower();

        // Ventanas del Stellar Launcher: no generan un item propio; el botón
        // fijo del lanzador ya refleja su estado a través del proceso.
        // "sgd" es la clase de las ventanas internas del propio dock
        // (incluido el popup de sistema), que tampoco generan items.
        if (wmClass == "stellar-launcher"  || wmClass == "stellar launcher" ||
            wmName  == "stellar-launcher"  || wmName  == "stellar launcher" ||
            wmClass == "sgd")
            continue;

        DockItem* match = nullptr;
        for (DockItem* it : m_items) {
            if (it->tipo() != DockItem::Tipo::Aplicacion) continue;
            InfoApp inf  = it->infoApp();
            QString iwm  = inf.wmClass.toLower();
            QString iname= inf.nombre.toLower();
            QString iexe = QFileInfo(inf.comandoExec.split(' ').first()).baseName().toLower();
            if ((!iwm.isEmpty() && (wmClass == iwm || wmName == iwm)) ||
                wmClass == iname || wmClass == iexe || wmName == iname || wmName == iexe)
            { match = it; break; }
        }

        if (!match) {
            InfoApp info = m_scanner->buscarPorWmClass(wmClass);
            if (info.nombre.isEmpty()) info = m_scanner->buscarPorWmClass(wmName);
            if (!info.nombre.isEmpty()) {
                bool yaExiste = false;
                for (DockItem* it : m_items)
                    if (it->infoApp().archivoDesktop == info.archivoDesktop)
                        { yaExiste = true; match = it; break; }
                if (!yaExiste) {
                    auto* newItem = new DockItem(info, this, this);
                    conectarItem(newItem);
                    newItem->show();
                    insertarItem(newItem);
                    layoutChanged = true;
                    match = newItem;
                }
            }
        }
        if (match) match->agregarVentana(wid);
    }

    // Limpiar ventanas muertas
    for (int i = m_items.size()-1; i >= 0; i--) {
        DockItem* it = m_items[i];
        if (it->tipo() != DockItem::Tipo::Aplicacion) continue;
        for (quint64 wid : it->ventanas())
            if (!actuales.contains(wid)) it->quitarVentana(wid);
        if (!it->estaAnclada() && !it->estaEjecutando()) {
            m_items.removeAt(i); it->deleteLater(); layoutChanged = true;
        }
    }

    // Ventana activa
    quint64 active = (quint64)x11_get_active_window();
    for (DockItem* it : m_items)
        it->setActiva(active && it->ventanas().contains(active));

    if (layoutChanged) { layoutItems(); posicionarDock(); }
}

// ─── Magnification ──────────────────────────────────────────────────────────

void DockWindow::actualizarMagnificacion(const QPoint& mouse) {
    if (!m_cfg.magnificacion) return;
    bool horizontal = (m_cfg.posicion == DockPosition::Abajo ||
                       m_cfg.posicion == DockPosition::Arriba);
    int base  = m_cfg.tamanoIcono;
    int maxSz = base * m_cfg.multiplicadorMag / 100;
    int rango = base * 2;
    for (DockItem* it : m_items) {
        if (it->tipo() == DockItem::Tipo::Separador ||
            it->tipo() == DockItem::Tipo::Reloj) continue;
        QPoint centro = it->mapToParent(it->rect().center());
        int dist = horizontal ? abs(mouse.x()-centro.x()) : abs(mouse.y()-centro.y());
        int sz;
        if (dist < rango) {
            double t = 1.0 - (double)dist/rango;
            t = t*t*(3-2*t);
            sz = base + (int)((maxSz-base)*t);
        } else sz = base;
        it->setTamanoMagnificado(sz);
    }
    layoutItems();
}

void DockWindow::resetMagnificacion() {
    for (DockItem* it : m_items) it->setTamanoMagnificado(0);
    layoutItems(); posicionarDock();
}

// ─── Auto-hide ──────────────────────────────────────────────────────────────

void DockWindow::mostrarDock() {
    if (!m_oculto) return;
    m_oculto = false;
    m_animSlide->stop();
    m_animSlide->setStartValue(m_posOffset);
    m_animSlide->setEndValue(0);
    m_animSlide->start();
}

void DockWindow::ocultarDock() {
    if (m_oculto || m_mouseAdentro) return;
    m_oculto = true;
    int grosor = m_cfg.tamanoIcono + 20 + m_cfg.padding * 2;
    m_animSlide->stop();
    m_animSlide->setStartValue(m_posOffset);
    m_animSlide->setEndValue(grosor - 3);
    m_animSlide->start();
}

// ─── Mouse ──────────────────────────────────────────────────────────────────

void DockWindow::enterEvent(QEnterEvent* e) {
    m_mouseAdentro = true;
    m_timerAutoOcultar->stop();
    if (m_cfg.comportamiento != DockComportamiento::SiempreVisible) mostrarDock();
    QWidget::enterEvent(e);
}

void DockWindow::leaveEvent(QEvent* e) {
    m_mouseAdentro = false;
    if (m_cfg.comportamiento != DockComportamiento::SiempreVisible) m_timerAutoOcultar->start();
    m_timerResetMag->start();
    QWidget::leaveEvent(e);
}

void DockWindow::mouseMoveEvent(QMouseEvent* e) {
    actualizarMagnificacion(e->pos());
    QWidget::mouseMoveEvent(e);
}

void DockWindow::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::RightButton) {
        bool sobreItem = false;
        for (DockItem* it : m_items)
            if (it->geometry().contains(e->pos())) { sobreItem = true; break; }
        if (!sobreItem) menuContextualDock(e->globalPosition().toPoint());
    }
    QWidget::mouseReleaseEvent(e);
}

// ─── Item Actions ────────────────────────────────────────────────────────────

void DockWindow::alHacerClicItem(DockItem* item) {
    if (item->tipo() == DockItem::Tipo::Lanzador) { abrirLanzador(); return; }
    if (item->tipo() == DockItem::Tipo::Reloj)    { abrirPanelSistema(item); return; }
    if (item->estaEjecutando()) {
        auto wins = item->ventanas();
        if (!wins.isEmpty()) x11_activate_window((unsigned long)wins.first());
    } else {
        item->lanzar();
    }
}

static QString estiloMenu();

void DockWindow::abrirPanelSistema(DockItem* item) {
    // El panel es un Qt::Popup: se crea una vez y se reutiliza. Se ancla junto
    // al rectángulo global del reloj (coordenadas de pantalla).
    if (!m_panelSistema) m_panelSistema = new SystemPanel(this);
    const QRect ancla(item->mapToGlobal(QPoint(0, 0)), item->size());
    m_panelSistema->abrirSobre(ancla, m_cfg.posicion);
}

void DockWindow::alHacerClicDerechoItem(DockItem* item, const QPoint& pos) {
    if (item->tipo() == DockItem::Tipo::Reloj) {
        QMenu menu;
        menu.setStyleSheet(estiloMenu());
        auto* hdr = menu.addAction(QStringLiteral("Reloj"));
        hdr->setEnabled(false);
        QFont f = hdr->font(); f.setBold(true); hdr->setFont(f);
        menu.addSeparator();
        menu.addAction(QStringLiteral("Preferencias del Dock…"), [this](){ abrirPreferencias(); });
        menu.exec(pos);
        return;
    }
    if (item->tipo() == DockItem::Tipo::Lanzador) {
        QMenu menu;
        menu.setStyleSheet(estiloMenu());
        auto* hdr = menu.addAction(QStringLiteral("Stellar Launcher"));
        hdr->setEnabled(false);
        QFont f = hdr->font(); f.setBold(true); hdr->setFont(f);
        menu.addSeparator();
        menu.addAction(QStringLiteral("Abrir"), [this](){ abrirLanzador(); });
        menu.addSeparator();
        menu.addAction(QStringLiteral("Preferencias del Dock…"), [this](){ abrirPreferencias(); });
        menu.exec(pos);
        return;
    }
    menuContextualItem(item, pos);
}

// ─── Context Menus ──────────────────────────────────────────────────────────

static QString estiloMenu() {
    return R"(
        QMenu {
            background-color: rgba(28,28,28,235);
            border: 1px solid rgba(255,255,255,50);
            border-radius: 10px;
            padding: 5px;
            color: white;
            font-size: 13px;
        }
        QMenu::item { padding: 7px 22px; border-radius: 5px; }
        QMenu::item:selected { background: rgba(255,255,255,35); }
        QMenu::item:disabled { color: rgba(255,255,255,100); }
        QMenu::separator { height:1px; background:rgba(255,255,255,25); margin:4px 10px; }
    )";
}

void DockWindow::menuContextualItem(DockItem* item, const QPoint& pos) {
    QMenu menu;
    menu.setStyleSheet(estiloMenu());

    auto* hdr = menu.addAction(item->infoApp().nombre);
    hdr->setEnabled(false);
    QFont f = hdr->font(); f.setBold(true); hdr->setFont(f);
    menu.addSeparator();

    if (item->estaAnclada())
        menu.addAction("Quitar del Dock", [this,item](){ desanclarApp(item->infoApp().archivoDesktop); });
    else
        menu.addAction("Mantener en el Dock", [this,item](){ anclarApp(item->infoApp().archivoDesktop); });

    if (!item->estaEjecutando()) {
        menu.addAction("Abrir", [item](){ item->lanzar(); });
    } else {
        menu.addAction("Nueva Ventana", [item](){ item->lanzar(); });

        auto wins = item->ventanas();
        if (wins.size() > 1) {
            menu.addSeparator();
            int n = 1;
            for (quint64 wid : wins) {
                char titulo[512] = {};
                if (!x11_get_window_title((unsigned long)wid, titulo, 512))
                    snprintf(titulo, 512, "Ventana %d", n);
                QString t = QString::fromUtf8(titulo);
                if (t.length() > 40) t = t.left(38) + "…";
                menu.addAction(t, [wid](){ x11_activate_window((unsigned long)wid); });
                n++;
            }
        }

        menu.addSeparator();
        menu.addAction("Cerrar", [item](){
            for (quint64 wid : item->ventanas())
                x11_close_window((unsigned long)wid);
        });
    }

    menu.addSeparator();
    menu.addAction("Preferencias del Dock…", [this](){ abrirPreferencias(); });
    menu.exec(pos);
}

void DockWindow::menuContextualDock(const QPoint& pos) {
    QMenu menu;
    menu.setStyleSheet(estiloMenu());
    menu.addAction("Preferencias del Dock…", [this](){ abrirPreferencias(); });
    QMenu* posMenu = menu.addMenu("Posición");
    posMenu->setStyleSheet(estiloMenu());
    posMenu->addAction("Abajo",     [this](){ DockConfig c=m_cfg; c.posicion=DockPosition::Abajo;     aplicarConfig(c); });
    posMenu->addAction("Arriba",    [this](){ DockConfig c=m_cfg; c.posicion=DockPosition::Arriba;    aplicarConfig(c); });
    posMenu->addAction("Izquierda", [this](){ DockConfig c=m_cfg; c.posicion=DockPosition::Izquierda; aplicarConfig(c); });
    posMenu->addAction("Derecha",   [this](){ DockConfig c=m_cfg; c.posicion=DockPosition::Derecha;   aplicarConfig(c); });
    menu.exec(pos);
}

void DockWindow::abrirPreferencias() {
    m_timerWM->stop();
    SettingsDialog dlg(m_cfg, m_scanner->todasLasApps(), m_items, this);
    if (dlg.exec() == QDialog::Accepted) {
        aplicarConfig(dlg.config());
        QStringList nuevasAncladas = dlg.appsAncladas();

        // Quitar desancladas
        for (DockItem* it : m_items) {
            if (it->tipo() != DockItem::Tipo::Aplicacion || !it->estaAnclada()) continue;
            if (!nuevasAncladas.contains(it->infoApp().archivoDesktop)) {
                it->setAnclada(false);
                if (!it->estaEjecutando()) { m_items.removeAll(it); it->deleteLater(); }
            }
        }
        // Agregar nuevas
        for (const QString& df : nuevasAncladas) {
            bool ya = false;
            for (DockItem* it : m_items)
                if (it->infoApp().archivoDesktop == df) { ya=true; it->setAnclada(true); break; }
            if (!ya) anclarApp(df);
        }
        layoutItems(); posicionarDock(); guardarAppsAncladas();
    }
    m_timerWM->start();
}

// ─── Lanzador de aplicaciones (Stellar Launcher) ─────────────────────────────

void DockWindow::abrirLanzador() {
    qint64 ahora = QDateTime::currentMSecsSinceEpoch();

    // Ya está abierto: el clic en el dock le quitó el foco y se cerró solo,
    // así que no se relanza (comportamiento toggle).
    if (m_procLanzador && m_procLanzador->state() != QProcess::NotRunning) return;
    if (m_lanzadorCerradoEn != 0 && ahora - m_lanzadorCerradoEn < 700) return;

    // Localizar el binario: en PATH (instalado), junto al dock, o en el árbol
    // de build durante el desarrollo.
    QString exe = QStandardPaths::findExecutable(QStringLiteral("stellar-launcher"));
    if (exe.isEmpty()) {
        QString junto = QCoreApplication::applicationDirPath() + "/stellar-launcher";
        if (QFile::exists(junto)) exe = junto;
    }
    if (exe.isEmpty()) {
        QString dev = QCoreApplication::applicationDirPath()
                      + "/launcher/stellarlauncher/stellar-launcher";
        if (QFile::exists(dev)) exe = dev;
    }
    if (exe.isEmpty()) {
        qWarning() << "SGD: no se encontró stellar-launcher en PATH ni junto al dock";
        return;
    }

    m_lanzadorCerradoEn = 0;

    if (!m_procLanzador) {
        m_procLanzador = new QProcess(this);
        connect(m_procLanzador, &QProcess::started,  this, &DockWindow::actualizarIndicadorLanzador);
        connect(m_procLanzador, &QProcess::finished, this, [this](int, QProcess::ExitStatus){
            m_lanzadorCerradoEn = QDateTime::currentMSecsSinceEpoch();
            actualizarIndicadorLanzador();
        });
    }

    m_procLanzador->start(exe, QStringList());
}

void DockWindow::actualizarIndicadorLanzador() {
    bool abierto = m_procLanzador && m_procLanzador->state() != QProcess::NotRunning;
    for (DockItem* it : m_items)
        if (it->tipo() == DockItem::Tipo::Lanzador) it->setEjecutando(abierto);
}
