#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

/**
 * Controlador del backend para las acciones de energía / sesión / ajustes.
 *
 * Expuesto a QML a través de la propiedad de contexto `systemController`.
 * Cada método se puede llamar directamente desde QML y comunica su resultado
 * mediante las señales `commandStarted` / `commandFailed`.
 */
class SystemController : public QObject
{
    Q_OBJECT

public:
    explicit SystemController(QObject *parent = nullptr);

    Q_INVOKABLE void powerOff();      // systemctl poweroff
    Q_INVOKABLE void reboot();        // systemctl reboot
    Q_INVOKABLE void suspend();       // systemctl suspend
    Q_INVOKABLE void lockSession();   // dm-tool switch-to-greeter | loginctl lock-session
    Q_INVOKABLE void openSettings();  // autodetecta la herramienta de ajustes del escritorio

signals:
    /** Se emite justo antes de iniciar un comando, con el nombre visible de la acción. */
    void commandStarted(const QString &action);
    /** Se emite cuando no se pudo iniciar un comando. */
    void commandFailed(const QString &action, const QString &message);

private:
    void execute(const QString &command, const QString &action);
    static QString firstExecutable(const QStringList &candidates);
};