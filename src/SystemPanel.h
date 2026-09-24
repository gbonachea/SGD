#pragma once
#include <QWidget>
#include <QString>
#include "DockConfig.h"

class QVBoxLayout;
class QLabel;
class QSlider;
class QPushButton;
class QWidget;

/**
 * Panel de control del sistema que se abre con clic sobre el reloj del dock:
 * redes WiFi, volumen y brillo. Se cierra solo al hacer clic fuera (Qt::Popup).
 *
 * Dependencias externas opcionales:
 *   - Red:      nmcli (NetworkManager)
 *   - Volumen:  pactl (PulseAudio/PipeWire) o amixer (ALSA)
 *   - Brillo:   brightnessctl o /sys/class/backlight
 */
class SystemPanel : public QWidget {
    Q_OBJECT
public:
    explicit SystemPanel(QWidget* parent = nullptr);

    // Abre el panel anclándolo junto al rectángulo del reloj (coordenadas globales).
    void abrirSobre(const QRect& ancla, DockPosition pos);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void construirUI();
    QLabel* seccionHeader(const QString& texto);

    QString ejecutar(const QString& cmd, int timeoutMs = 2500) const;

    // Red
    void actualizarRed();
    void rescanearRed();
    void poblarRed(const QString& out);
    void conectarWifi(const QString& ssid);
    void abrirConfigRed();

    // Volumen
    void actualizarVolumen();
    void setVolumen(int v);
    void cambiarVolumen(int delta);
    void alternarMute();

    // Brillo
    void actualizarBrillo();
    void setBrillo(int v);
    void cambiarBrillo(int delta);
    QString rutaBacklight() const;

    // Estado
    void setEstado(const QString& txt);

    QVBoxLayout* m_contenedor   = nullptr;
    QWidget*     m_zonaRed      = nullptr;
    QVBoxLayout* m_layoutRed    = nullptr;
    QLabel*      m_estado       = nullptr;

    QLabel*      m_labelVol     = nullptr;
    QSlider*     m_sliderVol    = nullptr;
    QPushButton* m_btnMute      = nullptr;

    QLabel*      m_labelBrillo  = nullptr;
    QSlider*     m_sliderBrillo = nullptr;

    bool         m_volPactl    = false;
    bool         m_brilloSysfs = false;
    bool         m_brilloCtl   = false;
    QString      m_backlightDir;
};