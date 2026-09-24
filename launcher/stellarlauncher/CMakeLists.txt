cmake_minimum_required(VERSION 3.16)

project(StellarLauncher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# ---------------------------------------------------------------------------
# Dependencias de Qt 6: Core + Gui (QProcess), Qml + Quick (motor e ítems QML)
# ---------------------------------------------------------------------------
find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(stellar-launcher
    src/main.cpp
    src/appmodel.h
    src/appmodel.cpp
    src/iconprovider.h
    src/iconprovider.cpp
    src/systemcontroller.h
    src/systemcontroller.cpp
)

# Se incrusta la interfaz QML como recursos para que el lanzador no dependa
# de un directorio de trabajo concreto en tiempo de ejecución.
qt_add_resources(stellar-launcher "qml"
    PREFIX "/"
    FILES
        qml/main.qml
        qml/VectorIcon.qml
        qml/PowerButton.qml
        qml/KeyBadge.qml
        qml/AboutDialog.qml
        icons/stellar-lanzador.png
)

target_link_libraries(stellar-launcher PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

install(TARGETS stellar-launcher RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})