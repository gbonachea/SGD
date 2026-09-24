#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QSortFilterProxyModel>
#include <QString>
#include <QStringList>

#include <optional>

class QSettings;

/**
 * Una aplicación instalada, tomada de un archivo `.desktop`.
 */
struct AppEntry
{
    QString name;        // Name localizado
    QString description; // GenericName (usa Comment como respaldo)
    QString exec;        // Línea Exec sin los códigos de campo (%U, %f, ...)
    QString icon;        // nombre del icono de tema, ruta absoluta o vacío
    QString id;          // id del archivo .desktop (nombre sin extensión)
};

/**
 * Modelo del backend que escanea los directorios estándar de `.desktop`
 * y expone las aplicaciones instaladas a QML
 * (expuesto a través de la propiedad de contexto `appModel`).
 */
class AppModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        DescriptionRole,
        ExecRole,
        IconRole,
        IdRole
    };

    explicit AppModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    /** Número total de aplicaciones indexadas (para el diálogo "Acerca de"). */
    int count() const { return rowCount(); }

    /** Vista filtrada + ordenada alfabéticamente de este modelo (para el ListView de QML). */
    QSortFilterProxyModel *proxy() const { return m_proxy; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void launchApp(const QString &execCommand) const;

signals:
    void countChanged();

private:
    QStringList desktopDirectories() const;
    std::optional<AppEntry> parseDesktopFile(const QString &filePath) const;
    QString localizedValue(QSettings &settings, const QString &key) const;
    QString sanitizeExec(QString exec) const;
    QString resolveIcon(const QString &icon, const QString &baseDir) const;
    static bool intersectsDesktopList(const QString &rawList,
                                      const QStringList &environments);

    QList<AppEntry> m_apps;
    QSortFilterProxyModel *m_proxy = nullptr;
};

/**
 * Proxy de filtrado/orden usado por el ListView de QML. El texto de búsqueda
 * se compara contra el nombre, la descripción y la línea Exec de la
 * aplicación (sin distinguir mayúsculas/minúsculas).
 */
class FilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit FilterProxyModel(QObject *parent = nullptr);

    Q_INVOKABLE void setFilterText(const QString &text);
    Q_INVOKABLE void clearFilter();
    Q_INVOKABLE QString execAt(int row) const;

    int count() const { return rowCount(); }

signals:
    void countChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_filterText;
};