#include "systemcontroller.h"

#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>

namespace {

/**
 * Inicia `command` totalmente desacoplado del lanzador.
 *
 * Los comandos simples (sin metacaracteres de shell) se dividen y se inician
 * directamente; cualquier otra cosa se entrega a `/bin/sh -c`, que también
 * cubre argumentos entre comillas y prefijos de variables de entorno como
 * `env FOO=1 app`.
 */
bool runDetached(const QString &command)
{
    const QString trimmed = command.trimmed();
    if (trimmed.isEmpty())
        return false;

    static const QRegularExpression shellChars(QStringLiteral(R"([|&;<>$`\\"'()])"));
    const bool needsShell = trimmed.contains(shellChars);

    if (!needsShell) {
        const QStringList parts = QProcess::splitCommand(trimmed);
        if (!parts.isEmpty()) {
            const QString program = parts.first();
            const QStringList args = parts.mid(1);
            if (QProcess::startDetached(program, args))
                return true;
        }
    }

    return QProcess::startDetached(QStringLiteral("/bin/sh"),
                                   { QStringLiteral("-c"), trimmed });
}

} // namespace

SystemController::SystemController(QObject *parent)
    : QObject(parent)
{
}

void SystemController::powerOff()
{
    execute(QStringLiteral("systemctl poweroff"), tr("Apagar"));
}

void SystemController::reboot()
{
    execute(QStringLiteral("systemctl reboot"), tr("Reiniciar"));
}

void SystemController::suspend()
{
    execute(QStringLiteral("systemctl suspend"), tr("Suspender"));
}

void SystemController::lockSession()
{
    // Se prefiere el saludo del gestor de pantalla; si no, bloqueo por logind.
    if (!QStandardPaths::findExecutable(QStringLiteral("dm-tool")).isEmpty()) {
        execute(QStringLiteral("dm-tool switch-to-greeter"), tr("Cambiar usuario"));
        return;
    }
    if (!QStandardPaths::findExecutable(QStringLiteral("loginctl")).isEmpty()) {
        execute(QStringLiteral("loginctl lock-session"), tr("Bloquear sesión"));
        return;
    }
    emit commandFailed(tr("Cambiar usuario"),
                       tr("No se encontró ni dm-tool ni loginctl en este sistema."));
}

void SystemController::openSettings()
{
    const QString executable = firstExecutable({
        QStringLiteral("gnome-control-center"),
        QStringLiteral("systemsettings"),
        QStringLiteral("xfce4-settings-manager"),
        QStringLiteral("cinnamon-settings"),
        QStringLiteral("mate-control-center"),
        QStringLiteral("lxqt-config"),
    });

    if (executable.isEmpty()) {
        emit commandFailed(tr("Ajustes"),
                           tr("No se encontró ninguna herramienta de ajustes en este sistema."));
        return;
    }

    execute(executable, tr("Ajustes"));
}

void SystemController::execute(const QString &command, const QString &action)
{
    if (runDetached(command)) {
        emit commandStarted(action);
    } else {
        emit commandFailed(action, tr("Error al ejecutar: %1").arg(command));
    }
}

QString SystemController::firstExecutable(const QStringList &candidates)
{
    for (const QString &candidate : candidates) {
        const QString path = QStandardPaths::findExecutable(candidate);
        if (!path.isEmpty())
            return candidate;
    }
    return {};
}