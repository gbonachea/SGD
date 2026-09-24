# Stellar — Lanzador de aplicaciones y menú de control (Qt6 / QML)

Un lanzador de aplicaciones moderno para Linux (estilo Rofi, pero con interfaz
gráfica flotante), con búsqueda en tiempo real y botones de energía/sesión
directos. Escrito en C++17 + Qt 6 (Qt Quick / QML) y diseñado para X11 y
Wayland.

![Paleta](https://img.shields.io/badge/palette-Catppuccin%20Mocha-1e1e2e)

## Captura

_Las capturas dependen del tema de iconos instalado en tu sistema._

## Requisitos

- Linux con Qt 6 (≥ 6.2): `qt6-base-dev`, `qt6-declarative-dev`
- CMake ≥ 3.16 y un compilador C++17 (GCC/Clang)
- (`libqt6svg6` recomendado para iconos SVG del tema)

## Compilación y ejecución

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/stellar-launcher
```

## Versión portátil

Se puede generar una carpeta autocontenida que incluye el binario, las
librerías Qt, los plugins y los módulos QML necesarios, ejecutable sin
instalar nada en el sistema:

```bash
./build-portable.sh          # genera la carpeta portable/
portable/stellar-launcher.sh # ejecuta la versión portátil
```

La carpeta `portable/` contiene:

```
portable/
├── stellar-launcher           binario
├── stellar-launcher.sh        lanzador (fija LD_LIBRARY_PATH y ejecuta)
├── qt.conf                    hace que Qt busque plugins/QML en ./plugins y ./qml
├── lib/                       ~140 librerías compartidas (Qt, X11, GL, ICU, …)
├── plugins/                   plugins Qt: plataformas X11/Wayland, GL, imágenes, iconos
└── qml/                       módulos QML usados (QtQuick, QtQml, …)
```

Detalles:

- **Independencia:** se empaquetan todas las librerías resueltas con `ldd`
  (paso recursivo sobre binario, plugins y módulos QML). Solo se excluyen la
  glibc y el cargador dinámico, que están garantizados en cualquier Linux de
  la misma arquitectura. Se incluye `libstdc++`, `libgcc_s`, X11, OpenGL y el
  resto para máxima portabilidad.
- **Plugins:** plataformas `xcb` (X11), `wayland` y `offscreen`, integraciones
  GL (EGL/GLX), formatos de imagen (`svg`, `jpeg`, `png`, …) y motor de iconos.
- **Módulos QML:** `QtQuick`, `QtQml` y sus submodulos (`Controls/Basic`,
  `Layouts`, `Window`, …), localizados por `qt.conf` de forma relativa al
  binario, por lo que la carpeta se puede mover o copiar a cualquier máquina.
- Un `tar.gz` compactado se genera como `stellar-portable.tar.gz`.
- *Nota:* los datos del sistema (tema de iconos, fuentes, XDG) se siguen
  usando del escritorio anfitrión, como es habitual en las apps portátiles.

## Uso

| Acción                     | Teclado / ratón                                        |
|----------------------------|--------------------------------------------------------|
| Buscar aplicaciones        | Escribe en el campo de búsqueda (filtrado en tiempo real) |
| Navegar por la lista       | `↑` / `↓` (y `RePág` / `AvPág`)                       |
| Lanzar la aplicación       | `Enter`, clic (selección directa) o botón `›` de la fila |
| Cerrar                     | `Esc`, botón `✕`, `Ctrl+Q` o **clic fuera de la ventana** |
| Ver información del app    | Clic en el logo ◆ (estrella junto al nombre)            |
| Mover la ventana           | Arrastrar la franja superior                           |
| Abrir config. del sistema  | Botón ⚙ (barra lateral “ACCIONES”)        |
| Suspender                  | Botón 🌙                                              |
| Cambiar de usuario / bloq. | Botón 👤 (barra lateral “SESIÓN”)                   |
| Reiniciar                  | Botón 🔄                                              |
| Apagar                     | Botón ⚡                                              |

## Características

- **Ventana flotante** sin bordes, siempre encima, centrada en pantalla,
  con sombra suave simulada y animación de entrada.
- **Panel izquierdo (búsqueda):**
  - Campo de texto con foco automático al abrir.
  - Filtrado en tiempo real sobre nombre, descripción y línea `Exec`
    (insensible a mayúsculas) usando un `QSortFilterProxyModel` personalizado.
  - `ListView` con icono + nombre (+ descripción), orden alfabético,
    resaltado por teclado y por hover, y arranque híbrido teclado/ratón.
- **Panel derecho (control):** botones con iconos vectoriales dibujados con
  `Canvas` (sin depender de fuentes emoji), colores de aviso (amarillo para
  reinicio, rojo para apagado), tooltips y un reloj en vivo.
- **Diálogo "Acerca de":** al hacer clic en el logo ◆ se abre un diálogo modal
  integrado en la propia ventana (sin crear una segunda ventana, para que el
  lanzador no se cierre al cambiar la activación) con la versión de la app y
  de Qt, el sistema operativo, la arquitectura y las aplicaciones detectadas.
  Se cierra con `Esc`, `✕`, el botón "Cerrar" o haciendo clic fuera de él.
- **Lectura de `.desktop`:** escanea `/usr/share/applications`,
  `/usr/local/share/applications`, `$XDG_DATA_DIRS`, `~/.local/share/applications`
  y exportaciones Flatpak. Soporta nombres localizados (`Name[es]`…),
  `NoDisplay`, `Hidden`, `OnlyShowIn`/`NotShowIn`, `TryExec`, iconos por tema
  o por ruta, y la eliminación de códigos de campo de `Exec` (`%U`, `%f`, …).
- **Lanzamiento robusto:** `QProcess::startDetached`; si la línea `Exec`
  contiene metacaracteres de shell se ejecuta vía `/bin/sh -c`.
- **Acciones del sistema:** `systemctl suspend|reboot|poweroff`, cambio de
  usuario con `dm-tool switch-to-greeter` (fallback `loginctl lock-session`) y
  detección automática de `gnome-control-center`, `systemsettings`,
  `xfce4-settings-manager`, etc. para abrir la configuración.
- **Iconos:** proveedor de imágenes (`Image` + `image://icons/…`) que resuelve
  nombres de tema y rutas de archivo, con icono genérico de respaldo.
- **Paleta Catppuccin Mocha** (`#1e1e2e`, `#b4befe`, …).

## Estructura

```
CMakeLists.txt            Configuración de compilación (Qt6 Core/Gui/Qml/Quick)
src/main.cpp              Punto de entrada: motor QML + vinculación del backend
src/appmodel.{h,cpp}      Escaneo/parseo de .desktop + modelo + proxy de filtrado
src/systemcontroller.{h,cpp}  Acciones de energía/sesión y ejecutor de procesos
src/iconprovider.{h,cpp}  QQuickImageProvider para iconos del tema y de archivo
qml/main.qml              Interfaz completa (búsqueda + sidebar de control)
qml/VectorIcon.qml        Iconos vectoriales propios (Canvas)
qml/PowerButton.qml       Botón de acción con tooltip
qml/KeyBadge.qml          Badge [tecla] + etiqueta para la barra de ayuda
qml/AboutDialog.qml       Diálogo "Acerca de" (información del aplicativo)
icons/stellar-lanzador.png Icono de la ventana/aplicación (incrustado en el binario)
build-portable.sh        Genera la versión portátil autocontenida (carpeta portable/)
portable/                Versión portátil generada por build-portable.sh
```

## Notas

- **Cierre al hacer clic fuera:** el lanzador se cierra en cuanto pierde la
  activación (foco): al hacer clic en otra ventana, en el panel/barra del
  sistema o en el escritorio. La detección se basa únicamente en eventos de
  foco, **sin tomar ningún *grab* global de ratón/teclado**, por lo que no
  puede bloquear el resto del escritorio y `Esc` / `✕` funcionan siempre.
  *Nota:* en gestores de ventanas con “focus sigue al ratón” o que no retiran
  el foco al pulsar sobre el fondo del escritorio, el clic en el escritorio
  vacío puede no cerrarla (en X11, un *grab* explícito estilo Rofi lo
  resolvería; consulta la sección de opciones).
- **Wayland/X11:** la ventana usa `Qt.FramelessWindowHint |
  Qt.WindowStaysOnTopHint` en ambas plataformas; Wayland la centra según la
  superficie, y el arrastre con ratón funciona vía geometría de ventana.
- Las acciones de `systemctl` requieren permisos de polkit en la sesión
  (el agente gráfico mostrará el diálogo de autorización).
- Añade una entrada `.desktop` o un atajo de teclado (ej. con *Custom Shortcut*
  de tu DE) apuntando a `build/stellar-launcher` para abrirlo al instante.
- Los iconos dependen del tema de iconos instalado (`hicolor`, `Papirus`, …).