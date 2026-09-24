#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QTimer>
#include <QPropertyAnimation>
#include "DockConfig.h"
#include "ApplicationScanner.h"

class DockItem;
class QProcess;
class SystemPanel;

class DockWindow : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int posOffset READ posOffset WRITE setPosOffset)

public:
    explicit DockWindow(QWidget* parent = nullptr);
    ~DockWindow();

    const DockConfig& config() const { return m_cfg; }
    void aplicarConfig(const DockConfig& cfg);

    // Items
    void anclarApp(const QString& desktopFile);
    void desanclarApp(const QString& desktopFile);
    void reconstruirDock();
    void insertarItem(DockItem* item);

    // Auto-hide property
    int  posOffset() const { return m_posOffset; }
    void setPosOffset(int v);

public slots:
    void abrirPreferencias();

protected:
    void paintEvent(QPaintEvent*) override;
    void enterEvent(QEnterEvent*) override;
    void leaveEvent(QEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void showEvent(QShowEvent*) override;

private:
    // Setup
    void cargarConfig();
    void guardarConfig();
    void cargarAppsAncladas();
    void guardarAppsAncladas();
    void configurarWM();
    void establecerEstrut();
    void limpiarEstrut();

    // Layout
    void layoutItems();
    void posicionarDock();

    // Window tracking
    void actualizarVentanas();

    // Magnification
    void actualizarMagnificacion(const QPoint& mouse);
    void resetMagnificacion();

    // Auto-hide
    void mostrarDock();
    void ocultarDock();

    // Menus
    void menuContextualItem(DockItem* item, const QPoint& pos);
    void menuContextualDock(const QPoint& pos);

    // Item signals
    void alHacerClicItem(DockItem* item);
    void alHacerClicDerechoItem(DockItem* item, const QPoint& pos);
    void conectarItem(DockItem* item);

    // Lanzador de aplicaciones (Stellar Launcher)
    void abrirLanzador();
    void actualizarIndicadorLanzador();

    // Panel de control del sistema (clic sobre el reloj): red, volumen, brillo
    void abrirPanelSistema(DockItem* item);

    // Screen geometry helper
    QRect geometriaDock() const;
    QScreen* pantallaObjetivo() const;

    DockConfig          m_cfg;
    QList<DockItem*>    m_items;
    ApplicationScanner* m_scanner;
    QSettings*          m_settings;

    // WM tracking
    QMap<quint64, DockItem*> m_ventanaAItem;
    QTimer*             m_timerWM;

    // Auto-hide
    QTimer*             m_timerAutoOcultar;
    QPropertyAnimation* m_animSlide;
    bool                m_oculto      = false;
    bool                m_mouseAdentro = false;
    int                 m_posOffset   = 0;

    // Magnification
    QTimer*             m_timerResetMag;

    // Screen update
    QTimer*             m_timerPantalla;

    // Stellar Launcher (proceso externo del lanzador de aplicaciones)
    QProcess*           m_procLanzador = nullptr;
    qint64              m_lanzadorCerradoEn = 0;

    // Panel de control del sistema (clic sobre el reloj)
    SystemPanel*        m_panelSistema = nullptr;
};
