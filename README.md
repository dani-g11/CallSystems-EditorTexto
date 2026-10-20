# Shell Educativo para la Exploración de Llamadas al Sistema (Linux)

Este proyecto es una herramienta diseñada en **C** para estudiantes de Sistemas Operativos. Consiste en una interfaz de línea de comandos (CLI) interactiva que clasifica y explica de forma visual las **llamadas al sistema (system calls)** básicas que realiza el kernel de Linux.

Cada comando educativo del shell simula o realiza operaciones comunes de un sistema operativo, y muestra en tiempo real (estilo `strace` simplificado) la llamada al sistema que ejecuta detrás de escena, incluyendo los parámetros que recibe y el valor devuelto.

\---

## Evaluación Práctica: Editor de Texto en Unix (Primer Parcial)

**Asignatura:** Sistemas Operativos (SO2026B) — Universidad EAFIT
**Tema:** Llamadas al sistema (System Calls) e interfaz POSIX (CLI) en Linux

Esta entrega extiende el shell educativo con un **editor de texto interactivo por líneas**,
integrado como un nuevo comando `editor` dentro del shell (categoría `edicion`). El editor abre un
sub-menú propio (`editor>`) y almacena el documento en una **lista enlazada** (una línea por nodo).
Todo el acceso a disco se realiza exclusivamente con llamadas al sistema de bajo nivel
(`open`, `read`, `write`, `lseek`, `ftruncate`, `close`), sin usar la familia `fopen`/`fread`/
`fwrite`/`fclose`.

Por tratarse de un equipo de **2 integrantes (parejas)**, además de los comandos base se
implementan los dos requisitos acumulativos del nivel: **inserción arbitraria** (`i \[n] \[texto]`) y
**búsqueda simple** (`s \[palabra]`).

### Integrantes del equipo

Daniel Mauricio Giraldo Moreno

Tomás Vera Franco

### Comandos del editor

|Comando|Descripción|
|-|-|
|`o \[archivo]`|Abre o crea un archivo y carga su contenido en la lista.|
|`a \[texto]`|Agrega una línea al final.|
|`p` / `p \[n]`|Imprime todo el documento, o solo la línea `n`.|
|`d \[n]`|Elimina la línea `n`.|
|`i \[n] \[texto]`|Inserta texto en la posición `n` (requisito de parejas).|
|`s \[palabra]`|Busca una palabra en el documento (requisito de parejas).|
|`q`|Guarda, cierra el archivo, libera memoria y vuelve al shell.|

### Cómo usar el editor

```bash
make            # compila y lanza el shell (binario eafitOS)
```

Dentro del shell:

```
editor notas.txt    # abre el editor cargando (o creando) notas.txt
```

\---

## Estructura de Comandos por Categoría

### 1\. Categoría: Datos y Archivos (`datos`)

Permite entender cómo interactúa el espacio de usuario con el sistema de archivos del kernel.

* `d\_create <archivo> "<texto>"`: Abre/crea un archivo y escribe un texto en él.

  * *Syscalls*: `open(2)`, `write(2)`, `close(2)`
* `d\_read <archivo>`: Abre un archivo para lectura, lee su contenido y lo imprime en pantalla.

  * *Syscalls*: `open(2)`, `read(2)`, `close(2)`
* `d\_info <archivo>`: Lee y detalla metadatos de un archivo como inodo, permisos, dueño y tamaño.

  * *Syscalls*: `stat(2)`
* `d\_copy <origen> <destino>`: Realiza una copia bloque por bloque de un archivo.

  * *Syscalls*: `open(2)`, `read(2)`, `write(2)`, `close(2)`

### 2\. Categoría: Memoria (`memoria`)

Explora el comportamiento de asignación dinámica de memoria del proceso del shell.

* `m\_sbrk <incremento>`: Ajusta el program break del proceso (heap). Útil para ver cómo crece y decrece el espacio de datos en bruto.

  * *Syscalls*: `sbrk(2)`
* `m\_mmap <tamaño\_bytes>`: Crea un mapeo de memoria anónima y privada en el espacio de direcciones, escribe un patrón de caracteres y luego la libera.

  * *Syscalls*: `mmap(2)`, `munmap(2)`
* `m\_info`: Muestra estadísticas detalladas del consumo y estado de memoria del propio shell.

  * *Mecanismo*: Lectura directa del archivo `/proc/self/status` del kernel.

### 3\. Categoría: Monitoreo y Procesos (`monitoreo`)

Visualiza la jerarquía de procesos, ejecución de programas y señales de Linux.

* `p\_fork`: Clona el proceso shell. El proceso hijo duerme 2 segundos y retorna con código `42`, mientras que el padre espera su término.

  * *Syscalls*: `fork(2)`, `getpid(2)`, `getppid(2)`, `waitpid(2)`
* `p\_exec <comando> \[args...]`: Es el núcleo de cualquier shell tradicional. Crea un hijo y este reemplaza su imagen de memoria con un ejecutable de Linux.

  * *Syscalls*: `fork(2)`, `execvp(3)`, `waitpid(2)`
* `p\_kill <pid> <numero\_señal>`: Envía una señal IPC a un proceso.

  * *Syscalls*: `kill(2)`
* `p\_monitor`: Muestra información estadística del uso de recursos del propio proceso (tiempos de CPU, fallos de página, etc.).

  * *Syscalls*: `getrusage(2)`

### 4\. Categoría: Utilidades (`utilidades`)

Comandos útiles para ver la hora, la fecha y saludar al usuario actual.

* `saludar`: Saluda al usuario actual leyendo su información de UID.

  * *Syscalls*: `getuid(2)`
* `hora`: Muestra la hora del sistema en formato HH:MM:SS.

  * *Syscalls*: `time(2)`
* `fecha`: Muestra la fecha del sistema en formato YYYY-MM-DD.

  * *Syscalls*: `time(2)`

\---

## Compilación y Ejecución

Para compilar el proyecto es necesario estar en un entorno Linux con GCC y Make instalados.

1. **Compilar el proyecto:**

```bash
   make
   ```

   Esto producirá el binario ejecutable `sys\_shell`.

2. **Iniciar el shell interactivo:**

```bash
   ./sys\_shell
   ```

3. **Limpiar archivos objeto y binarios:**

```bash
   make clean
   ```

\---

## Ejemplo de Uso Educativo

Cuando ejecutas un comando en `sys\_shell`, verás las syscalls ejecutadas en color **magenta** en la terminal:

```ansi
sys-shell> d\_create mi\_archivo.txt "Hola SO"
\[syscall] open("mi\_archivo.txt", O\_WRONLY|O\_CREAT|O\_TRUNC, 0644) ... = 3
\[syscall] write(3, "Hola SO", 7) ... = 7
\[syscall] close(3) ... = 0
Archivo 'mi\_archivo.txt' creado exitosamente con 7 bytes.

sys-shell> d\_info mi\_archivo.txt
\[syscall] stat("mi\_archivo.txt", \&st) ... = 0
--- Metadatos del Archivo (stat) ---
  Inodo:          456789
  Permisos (oct): 644
  Tamaño:         7 bytes
  MTime (Modif):  Mon Jul 27 13:50:00 2026
------------------------------------
```

\---

## Ayuda Interactiva del Shell

El shell cuenta con una sección de ayuda dinámica muy completa. Puedes escribir:

* `help`: Muestra la bienvenida y el listado de categorías.
* `help <categoria>`: Muestra todos los comandos pertenecientes a `datos`, `memoria`, `monitoreo` o `utilidades`.
* `help <comando>`: Explica individualmente qué hace un comando, cómo se usa y qué llamadas al sistema específicas invoca.

