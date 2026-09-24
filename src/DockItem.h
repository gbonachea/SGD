#pragma once
#include <QWidget>
#include <QPropertyAnimation>
#include <QTimer>
#include "DockConfig.h"
#include "ApplicationScanner.h"

class DockWindow;

class DockItem : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal escala   READ escala   WRITE setEscala)
    Q_PROPERTY(qreal rebote   READ rebote   WRITE setRebote)

public:
    enum class Tipo { Aplicacion, Separador, Lanzador, Reloj };

    explicit DockItem(const InfoApp& app, DockWindow* dock, QWidget* parent = nullptr);
    explicit DockItem(Tipo tipo,          DockWindow* dock, QWidget* parent = nullptr);

    InfoApp     infoApp() const          { return m_app; }
    Tipo        tipo() const             { return m_tipo; }
    bool        estaAnclada() const      { return m_app.estaAnclada; }
    bool        estaEjecutando() const   { return m_app.estaEjecutando; }
    QList<quint64> ventanas() const      { return m_app.ventanas; }
    int         tamanoActual() const;

    void setEjecutando(bool v)           { m_app.estaEjecutando = v; update(); }
    void setIcono(const QIcon& ic)       { m_app.icono = ic; update(); }
    void setNombre(const QString& n)     { m_app.nombre = n; }
    void agregarVentana(quint64 id);
    void quitarVentana(quint64 id);
    void setActiva(bool v)               { m_activa = v; update(); }
    void setAnclada(bool v)              { m_app.estaAnclada = v; }
    void setTamanoMagnificado(int sz)    { m_tamMag = sz; updateGeometry(); update(); }

    // Animation props
    qreal escala() const  { return m_escala; }
    void  setEscala(qreal v) { m_escala = v; update(); }
    qreal rebote() const  { return m_rebote; }
    void  setRebote(qreal v) { m_rebote = v; update(); }

    void lanzar();
    void animarRebote();

    // Public size so DockWindow can query without breaking encapsulation
    QSize tamano() const;

signals:
    void clicIzquierdo();
    void clicDerecho(QPoint pos);
    void clicMedio();

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void enterEvent(QEnterEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    void dibujarIndicadores(QPainter& p, const QRect& rc);
    void dibujarEtiqueta(QPainter& p);
    void actualizarBateria();

    InfoApp     m_app;
    Tipo        m_tipo;
    DockWindow* m_dock;

    qreal  m_escala  = 1.0;
    qreal  m_rebote  = 0.0;
    bool   m_hover   = false;
    bool   m_presion = false;
    bool   m_activa  = false;
    int    m_tamMag  = 0;
    bool   m_mostrarEtiqueta = false;

    QPropertyAnimation* m_animEscala = nullptr;
    QPropertyAnimation* m_animRebote = nullptr;
    QTimer*             m_timerEtiqueta = nullptr;
    QTimer*             m_timerRebote   = nullptr;
    QTimer*             m_timerReloj    = nullptr;
    int                 m_contRebote    = 0;

    // Indicador de batería (contorno del reloj); -1 = sin batería detectada
    int  m_bateriaNivel    = -1;
    bool m_bateriaCargando = false;
};
