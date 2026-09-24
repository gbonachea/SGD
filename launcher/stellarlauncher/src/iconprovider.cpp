#include "iconprovider.h"

#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QUrl>

IconImageProvider::IconImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap IconImageProvider::requestPixmap(const QString &id, QSize *size,
                                         const QSize &requestedSize)
{
    const int target = (requestedSize.isValid() && requestedSize.width() > 0)
                           ? requestedSize.width()
                           : 64;

    const QString value = QUrl::fromPercentEncoding(id.toUtf8());

    if (value.isEmpty())
        return fallbackPixmap(target);

    // 1) Ruta absoluta (ya resuelta por AppModel) -> se carga el archivo.
    QString path = value;
    if (path.startsWith(QStringLiteral("~")))
        path = QDir::homePath() + path.mid(1);
    if (QFileInfo::exists(path)) {
        const QPixmap pixmap(path);
        if (!pixmap.isNull()) {
            if (size)
                *size = pixmap.size();
            return pixmap;
        }
    }

    // 2) Nombre de icono de tema.
    if (!value.contains(QLatin1Char('/'))) {
        const QIcon icon = QIcon::fromTheme(value);
        if (!icon.isNull()) {
            const QPixmap pixmap = icon.pixmap(target, target);
            if (!pixmap.isNull()) {
                if (size)
                    *size = pixmap.size();
                return pixmap;
            }
        }
    }

    // 3) Respaldo genérico para que la interfaz nunca muestre una imagen rota.
    return fallbackPixmap(target);
}

QPixmap IconImageProvider::fallbackPixmap(int size)
{
    const QIcon fallback = QIcon::fromTheme(QStringLiteral("application-x-executable"));
    if (!fallback.isNull()) {
        const QPixmap pixmap = fallback.pixmap(size, size);
        if (!pixmap.isNull())
            return pixmap;
    }

    // Último recurso: un píxel totalmente transparente (sin error, sin fallo).
    QPixmap transparent(1, 1);
    transparent.fill(Qt::transparent);
    return transparent;
}