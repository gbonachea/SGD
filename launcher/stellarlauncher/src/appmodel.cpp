#include "appmodel.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QLocale>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>

// ---------------------------------------------------------------------------
// AppModel
// ---------------------------------------------------------------------------

AppModel::AppModel(QObject *parent)
    : QAbstractListModel(parent)
{
    refresh();

    // El proxy pertenece al modelo pero se expone a QML como `appProxy`.
    m_proxy = new FilterProxyModel(this);
    m_proxy->setSourceModel(this);
}

int AppModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_apps.size();
}

QVariant AppModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_apps.size())
        return {};

    const AppEntry &app = m_apps.at(index.row());
    switch (role) {
    case NameRole:        return app.name;
    case DescriptionRole: return app.description;
    case ExecRole:        return app.exec;
    case IconRole:        return app.icon;
    case IdRole:          return app.id;
    default:              return {};
    }
}

QHash<int, QByteArray> AppModel::roleNames() const
{
    return {
        { NameRole, "name" },
        { DescriptionRole, "description" },
        { ExecRole, "exec" },
        { IconRole, "icon" },
        { IdRole, "id" },
    };
}

void AppModel::refresh()
{
    beginResetModel();
    m_apps.clear();

    // id -> índice en m_apps. Los directorios se escanean primero el del
    // sistema y el del usuario al final, de modo que un override del usuario
    // sustituye a la entrada del sistema.
    QHash<QString, int> seen;

    const QStringList directories = desktopDirectories();
    for (const QString &directory : directories) {
        const QDir dir(directory);
        if (!dir.exists())
            continue;

        const QFileInfoList files = dir.entryInfoList({ QStringLiteral("*.desktop") },
                                                      QDir::Files);
        for (const QFileInfo &info : files) {
            const auto entry = parseDesktopFile(info.absoluteFilePath());
            if (!entry)
                continue;

            const QString id = info.completeBaseName();
            if (seen.contains(id)) {
                m_apps[seen.value(id)] = *entry;
            } else {
                seen.insert(id, m_apps.size());
                m_apps.append(*entry);
            }
        }
    }

    endResetModel();
    emit countChanged();
    qInfo() << "Stellar Launcher:" << m_apps.size() << "aplicaciones indexadas";
}

void AppModel::launchApp(const QString &execCommand) const
{
    const QString command = execCommand.trimmed();
    if (command.isEmpty())
        return;

    const QString cleaned = sanitizeExec(command);

    // Si el comando contiene metacaracteres de shell se entrega a /bin/sh.
    // En caso contrario se analiza y se arranca totalmente desacoplado.
    static const QRegularExpression shellChars(QStringLiteral(R"([|&;<>$`\\"'()])"));
    const bool needsShell = cleaned.contains(shellChars);

    if (!needsShell) {
        const QStringList parts = QProcess::splitCommand(cleaned);
        if (!parts.isEmpty()) {
            const QString program = parts.first();
            const QStringList args = parts.mid(1);
            if (!program.isEmpty() && QProcess::startDetached(program, args))
                return;
            qWarning() << "No se pudo iniciar el programa desacoplado:" << program;
        }
    }

    QProcess::startDetached(QStringLiteral("/bin/sh"),
                            { QStringLiteral("-c"), cleaned });
}

QStringList AppModel::desktopDirectories() const
{
    QStringList dirs;
    const auto addUnique = [&dirs](const QString &dir) {
        const QString clean = QDir::cleanPath(dir);
        if (!clean.isEmpty() && !dirs.contains(clean))
            dirs.append(clean);
    };

    // Ubicaciones de todo el sistema
    addUnique(QStringLiteral("/usr/share/applications"));
    addUnique(QStringLiteral("/usr/local/share/applications"));

    // Ubicación por usuario (XDG_DATA_HOME/applications)
    QString dataHome = qEnvironmentVariable("XDG_DATA_HOME");
    if (dataHome.isEmpty())
        dataHome = QDir::homePath() + QStringLiteral("/.local/share");
    addUnique(dataHome + QStringLiteral("/applications"));

    // Todo lo listado en XDG_DATA_DIRS
    const QStringList dataDirs =
        qEnvironmentVariable("XDG_DATA_DIRS").split(QLatin1Char(':'), Qt::SkipEmptyParts);
    for (const QString &dataDir : dataDirs)
        addUnique(dataDir + QStringLiteral("/applications"));

    // Exportaciones de Flatpak (sistema + usuario)
    addUnique(QStringLiteral("/var/lib/flatpak/exports/share/applications"));
    addUnique(QStringLiteral("/usr/share/flatpak/exports/share/applications"));
    addUnique(QDir::homePath() + QStringLiteral("/.local/share/flatpak/exports/share/applications"));

    return dirs;
}

std::optional<AppEntry> AppModel::parseDesktopFile(const QString &filePath) const
{
    QSettings settings(filePath, QSettings::IniFormat);

    settings.beginGroup(QStringLiteral("Desktop Entry"));

    const QString type = settings.value(QStringLiteral("Type")).toString();
    const bool hidden = settings.value(QStringLiteral("Hidden"))
                            .toString().compare(QLatin1String("true"), Qt::CaseInsensitive) == 0;
    const bool noDisplay = settings.value(QStringLiteral("NoDisplay"))
                               .toString().compare(QLatin1String("true"), Qt::CaseInsensitive) == 0;

    if (type.compare(QLatin1String("Application"), Qt::CaseInsensitive) != 0
        || hidden || noDisplay) {
        return std::nullopt;
    }

    const QString execRaw = settings.value(QStringLiteral("Exec")).toString().trimmed();
    if (execRaw.isEmpty())
        return std::nullopt;

    // Filtrado por OnlyShowIn / NotShowIn
    const QString currentDesktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
    const QStringList environments =
        currentDesktop.split(QLatin1Char(':'), Qt::SkipEmptyParts);
    const QString onlyShowIn = settings.value(QStringLiteral("OnlyShowIn")).toString();
    const QString notShowIn = settings.value(QStringLiteral("NotShowIn")).toString();
    if (!onlyShowIn.isEmpty() && !intersectsDesktopList(onlyShowIn, environments))
        return std::nullopt;
    if (!notShowIn.isEmpty() && intersectsDesktopList(notShowIn, environments))
        return std::nullopt;

    // TryExec: se omite la entrada cuando el ejecutable no existe
    const QString tryExec = settings.value(QStringLiteral("TryExec")).toString().trimmed();
    if (!tryExec.isEmpty()
        && QStandardPaths::findExecutable(tryExec.section(QLatin1Char(' '), 0, 0)).isEmpty()) {
        return std::nullopt;
    }

    AppEntry entry;
    entry.name = localizedValue(settings, QStringLiteral("Name")).trimmed();
    if (entry.name.isEmpty())
        return std::nullopt;

    entry.description = localizedValue(settings, QStringLiteral("GenericName")).trimmed();
    if (entry.description.isEmpty())
        entry.description = settings.value(QStringLiteral("Comment")).toString().trimmed();

    entry.exec = sanitizeExec(execRaw);
    if (entry.exec.isEmpty())
        return std::nullopt;

    entry.icon = resolveIcon(settings.value(QStringLiteral("Icon")).toString(),
                             QFileInfo(filePath).absolutePath());
    entry.id = QFileInfo(filePath).completeBaseName();

    return entry;
}

QString AppModel::localizedValue(QSettings &settings, const QString &key) const
{
    const QString locale = QLocale::system().name();          // p. ej. "es_ES"
    const QString shortLocale = locale.section(QLatin1Char('_'), 0, 0); // p. ej. "es"

    const QString localized =
        settings.value(QStringLiteral("%1[%2]").arg(key, locale)).toString();
    if (!localized.isEmpty())
        return localized;

    const QString shortLocalized =
        settings.value(QStringLiteral("%1[%2]").arg(key, shortLocale)).toString();
    if (!shortLocalized.isEmpty())
        return shortLocalized;

    return settings.value(key).toString();
}

QString AppModel::sanitizeExec(QString exec) const
{
    exec = exec.trimmed();

    // Divide en tokens preservando los segmentos entre comillas
    QStringList tokens;
    QString current;
    QChar quote = QChar::Null;
    for (const QChar &ch : exec) {
        if (quote.isNull() && (ch == QLatin1Char('\'') || ch == QLatin1Char('"'))) {
            quote = ch;
            current += ch;
        } else if (!quote.isNull() && ch == quote) {
            quote = QChar::Null;
            current += ch;
        } else if (quote.isNull() && ch.isSpace()) {
            if (!current.isEmpty()) {
                tokens.append(current);
                current.clear();
            }
        } else {
            current += ch;
        }
    }
    if (!current.isEmpty())
        tokens.append(current);

    QStringList kept;
    for (QString &token : tokens) {
        token.replace(QStringLiteral("%%"), QStringLiteral("\u0001")); // protege el '%' literal
        token.remove(QRegularExpression(QStringLiteral("%[a-zA-Z]"))); // elimina los códigos de campo
        token.replace(QStringLiteral("\u0001"), QStringLiteral("%"));
        if (!token.isEmpty())
            kept.append(token);
    }

    return kept.join(QLatin1Char(' '));
}

QString AppModel::resolveIcon(const QString &icon, const QString &baseDir) const
{
    if (icon.isEmpty())
        return {};

    // Ruta absoluta (o relativa al home) -> se usa el archivo directamente
    if (icon.startsWith(QLatin1Char('/')) || icon.startsWith(QStringLiteral("~"))) {
        QString path = icon;
        if (path.startsWith(QStringLiteral("~")))
            path = QDir::homePath() + path.mid(1);
        if (QFileInfo::exists(path))
            return path;
        return {};
    }

    // Icono junto al archivo .desktop (algunas aplicaciones antiguas lo hacen)
    if (QFileInfo::exists(baseDir + QLatin1Char('/') + icon))
        return baseDir + QLatin1Char('/') + icon;

    // Nombre de icono de tema -> se conserva, QML lo resuelve mediante el tema
    if (QIcon::hasThemeIcon(icon))
        return icon;

    const QString bare = icon.section(QLatin1Char('.'), 0, 0); // quita cualquier extensión
    if (bare != icon && QIcon::hasThemeIcon(bare))
        return bare;

    return {};
}

bool AppModel::intersectsDesktopList(const QString &rawList,
                                     const QStringList &environments)
{
    const QStringList entries =
        rawList.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &entry : entries) {
        for (const QString &env : environments) {
            if (env.compare(entry.trimmed(), Qt::CaseInsensitive) == 0)
                return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// FilterProxyModel
// ---------------------------------------------------------------------------

FilterProxyModel::FilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setSortLocaleAware(true);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortRole(AppModel::NameRole);
    sort(0, Qt::AscendingOrder);
}

void FilterProxyModel::setFilterText(const QString &text)
{
    const QString needle = text.trimmed().toLower();
    if (needle == m_filterText)
        return;

    m_filterText = needle;
    invalidateFilter();
    emit countChanged();
}

void FilterProxyModel::clearFilter()
{
    setFilterText(QString());
}

QString FilterProxyModel::execAt(int row) const
{
    if (row < 0 || row >= rowCount())
        return {};

    const QModelIndex source = mapToSource(index(row, 0));
    if (!source.isValid())
        return {};

    return sourceModel()->data(source, AppModel::ExecRole).toString();
}

bool FilterProxyModel::filterAcceptsRow(int sourceRow,
                                        const QModelIndex &sourceParent) const
{
    if (m_filterText.isEmpty())
        return true;

    const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!index.isValid())
        return false;

    const QString name = sourceModel()->data(index, AppModel::NameRole).toString();
    const QString exec = sourceModel()->data(index, AppModel::ExecRole).toString();
    const QString description =
        sourceModel()->data(index, AppModel::DescriptionRole).toString();

    return name.contains(m_filterText, Qt::CaseInsensitive)
        || exec.contains(m_filterText, Qt::CaseInsensitive)
        || description.contains(m_filterText, Qt::CaseInsensitive);
}