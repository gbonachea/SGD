#pragma once

#include <QQuickImageProvider>
#include <QPixmap>

/**
 * Proveedor de imágenes registrado en el motor QML como "icons".
 *
 * Uso en QML: Image { source: "image://icons/" + encodeURIComponent(valor) }
 *
 * `valor` puede ser:
 *   - un nombre de icono de tema     (resuelto mediante QIcon::fromTheme)
 *   - una ruta de archivo absoluta   (cargada directamente)
 *
 * Si no se puede resolver nada, se devuelve un icono de aplicación genérico
 * para que la interfaz nunca muestre un marcador de imagen rota.
 */
class IconImageProvider : public QQuickImageProvider
{
public:
    explicit IconImageProvider();

    QPixmap requestPixmap(const QString &id, QSize *size,
                          const QSize &requestedSize) override;

private:
    static QPixmap fallbackPixmap(int size);
};