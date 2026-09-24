#include <QCoreApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSysInfo>
#include <QUrl>

#include "appmodel.h"
#include "iconprovider.h"
#include "systemcontroller.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("Stellar Launcher"));
    QCoreApplication::setOrganizationName(QStringLiteral("Stellar"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    // Icono por defecto del lanzador (incrustado como recurso).
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/stellar-lanzador.png")));

    // Objetos del backend (deben sobrevivir al motor).
    AppModel appModel;
    SystemController systemController;

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty(QStringLiteral("appModel"), &appModel);
    engine.rootContext()->setContextProperty(QStringLiteral("appProxy"),
                                             appModel.proxy());
    engine.rootContext()->setContextProperty(QStringLiteral("systemController"),
                                             &systemController);

    // Información del aplicativo para el diálogo "Acerca de" (versión, Qt,
    // sistema operativo y arquitectura), consultada desde el QML.
    engine.rootContext()->setContextProperty(QStringLiteral("aboutAppVersion"),
                                             QCoreApplication::applicationVersion());
    engine.rootContext()->setContextProperty(QStringLiteral("aboutQtVersion"),
                                             QString::fromLatin1(qVersion()));
    engine.rootContext()->setContextProperty(QStringLiteral("aboutOsName"),
                                             QSysInfo::prettyProductName());
    engine.rootContext()->setContextProperty(QStringLiteral("aboutArch"),
                                             QSysInfo::currentCpuArchitecture());

    // Resolución de iconos: iconos de tema + rutas de archivo a través de un
    // único proveedor de imágenes.
    engine.addImageProvider(QStringLiteral("icons"), new IconImageProvider());

    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}