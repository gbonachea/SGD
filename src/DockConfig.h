#pragma once
#include <QString>
#include <QColor>

enum class DockPosition { Abajo, Arriba, Izquierda, Derecha };
enum class DockComportamiento { SiempreVisible, AutoOcultar };

struct DockConfig {
    DockPosition       posicion           = DockPosition::Abajo;
    DockComportamiento comportamiento      = DockComportamiento::SiempreVisible;
    int                tamanoIcono        = 32;
    bool               magnificacion      = true;
    int                multiplicadorMag   = 160;   // % del tamaño base
    bool               fondoTranslucido   = true;
    double             opacidadFondo      = 0.80;
    bool               mostrarIndicadores = true;
    bool               mostrarEtiquetas   = true;
    bool               rebotarAlAbrir     = true;
    int                indiceMonitor      = 0;
    int                radioEsquina       = 16;
    int                espaciadoItems     = 6;
    int                padding            = 10;
    int                margen             = 4;
};
