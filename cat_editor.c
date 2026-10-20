#include "shell.h"
#include <unistd.h>      /* read, write, lseek, ftruncate, close */
#include <fcntl.h>       /* open, O_RDWR, O_CREAT */
#include <sys/types.h>   /* off_t, ssize_t */
#include <string.h>      /* strlen, strcpy, strstr */
#include <stdlib.h>      /* malloc, free */

/**
 * ====================================================================================
 * MÓDULO: EDITOR DE TEXTO POR LÍNEAS (categoría "edicion")
 * ====================================================================================
 * Este módulo agrega al shell educativo un pequeño editor de texto que guarda el
 * contenido en una LISTA ENLAZADA SIMPLE (una línea de texto por nodo).
 *
 * Idea general (para estudiantes nuevos en C):
 *   - La lista enlazada que vive en memoria RAM es la "fuente de la verdad".
 *   - Cada vez que modificamos la lista (agregar, insertar o borrar una línea),
 *     volvemos a escribir TODO el contenido en el archivo del disco.
 *   - Para tocar el disco usamos SOLO llamadas al sistema de bajo nivel:
 *       open, read, write, lseek, ftruncate y close.
 *     (Está PROHIBIDO usar fopen/fread/fwrite/fclose en este proyecto.)
 *
 * El editor funciona como un "mini shell" dentro del shell: muestra su propio
 * prompt "editor> " y acepta comandos de una sola letra hasta que escribimos 'q'.
 *
 * Subcomandos disponibles:
 *   o [archivo]      Abre o crea un archivo y carga su contenido en la lista.
 *   a [texto]        Agrega una línea nueva al final.
 *   p                Imprime todo el texto (líneas numeradas desde 1).
 *   p [n]            Imprime solo la línea número n.
 *   d [n]            Elimina la línea número n.
 *   i [n] [texto]    Inserta "texto" como nueva línea en la posición n.
 *   s [palabra]      Busca una palabra y muestra en qué líneas aparece.
 *   h                Muestra la ayuda del editor.
 *   q                Guarda, cierra el archivo, libera memoria y vuelve al shell.
 */

/* Longitud máxima de una línea de entrada del teclado dentro del editor */
#define EDITOR_MAX_LINEA 2048

/**
 * ------------------------------------------------------------------------------------
 * Estructura del nodo de la lista enlazada.
 * ------------------------------------------------------------------------------------
 * Cada nodo guarda UNA línea de texto (sin el salto de línea '\n') y un puntero
 * al siguiente nodo. El último nodo apunta a NULL.
 */
typedef struct Linea {
    char *texto;          /* línea SIN el '\n', terminada en '\0' */
    struct Linea *sig;    /* puntero al siguiente nodo (NULL si es el último) */
} Linea;

/* ====================================================================================
 * FUNCIONES AUXILIARES DE LA LISTA ENLAZADA
 * ==================================================================================== */

/**
 * Crea un nodo nuevo copiando el texto recibido.
 * Reserva su propia memoria con malloc para que la línea no dependa del buffer
 * original del teclado. Devuelve NULL si no hay memoria.
 */
Linea *nueva_linea(const char *texto) {
    Linea *nodo = (Linea *)malloc(sizeof(Linea));
    if (nodo == NULL) {
        return NULL;
    }
    /* +1 para reservar espacio para el terminador '\0' */
    nodo->texto = (char *)malloc(strlen(texto) + 1);
    if (nodo->texto == NULL) {
        free(nodo);
        return NULL;
    }
    strcpy(nodo->texto, texto);
    nodo->sig = NULL;
    return nodo;
}

/**
 * Agrega una línea nueva AL FINAL de la lista.
 * Recibe la dirección de la cabeza (Linea **) porque si la lista está vacía
 * debemos poder modificar el puntero cabeza del llamador.
 */
void agregar_final(Linea **cabeza, const char *texto) {
    Linea *nodo = nueva_linea(texto);
    if (nodo == NULL) {
        printf(COLOR_ERROR "Error: sin memoria para agregar la línea.\n" COLOR_RESET);
        return;
    }
    if (*cabeza == NULL) {
        /* La lista estaba vacía: el nodo nuevo pasa a ser la cabeza */
        *cabeza = nodo;
        return;
    }
    /* Recorrer hasta el último nodo y enlazar el nuevo al final */
    Linea *actual = *cabeza;
    while (actual->sig != NULL) {
        actual = actual->sig;
    }
    actual->sig = nodo;
}

/**
 * Cuenta cuántas líneas (nodos) tiene la lista. Útil para validar rangos.
 */
int contar_lineas(Linea *cabeza) {
    int total = 0;
    Linea *actual = cabeza;
    while (actual != NULL) {
        total++;
        actual = actual->sig;
    }
    return total;
}

/**
 * Inserta "texto" como una línea nueva en la posición n (contando desde 1),
 * desplazando hacia abajo el resto de las líneas.
 *   - n = 1 inserta al inicio.
 *   - n = (total + 1) inserta al final.
 * Devuelve 1 si lo logró, 0 si la posición es inválida.
 */
int insertar_en(Linea **cabeza, int n, const char *texto) {
    int total = contar_lineas(*cabeza);

    /* La posición válida va de 1 hasta total+1 (una después de la última) */
    if (n < 1 || n > total + 1) {
        return 0;
    }

    Linea *nodo = nueva_linea(texto);
    if (nodo == NULL) {
        printf(COLOR_ERROR "Error: sin memoria para insertar la línea.\n" COLOR_RESET);
        return 0;
    }

    if (n == 1) {
        /* Insertar al inicio: el nodo nuevo apunta a la antigua cabeza */
        nodo->sig = *cabeza;
        *cabeza = nodo;
        return 1;
    }

    /* Avanzar hasta el nodo anterior a la posición n (el nodo n-1) */
    Linea *anterior = *cabeza;
    for (int i = 1; i < n - 1; i++) {
        anterior = anterior->sig;
    }
    /* Enlazar: anterior -> nodo -> (lo que seguía a anterior) */
    nodo->sig = anterior->sig;
    anterior->sig = nodo;
    return 1;
}

/**
 * Elimina la línea número n (contando desde 1) y libera su memoria.
 * Devuelve 1 si la eliminó, 0 si n está fuera de rango.
 */
int eliminar_linea(Linea **cabeza, int n) {
    int total = contar_lineas(*cabeza);
    if (n < 1 || n > total) {
        return 0;
    }

    Linea *a_borrar;

    if (n == 1) {
        /* Borrar la cabeza: la nueva cabeza es el segundo nodo */
        a_borrar = *cabeza;
        *cabeza = (*cabeza)->sig;
    } else {
        /* Avanzar hasta el nodo anterior (n-1) para "saltar" el nodo n */
        Linea *anterior = *cabeza;
        for (int i = 1; i < n - 1; i++) {
            anterior = anterior->sig;
        }
        a_borrar = anterior->sig;
        anterior->sig = a_borrar->sig;
    }

    /* Liberar primero el texto y luego el nodo (evita fugas de memoria) */
    free(a_borrar->texto);
    free(a_borrar);
    return 1;
}

/**
 * Imprime el contenido de la lista.
 *   - Si n <= 0 imprime TODAS las líneas numeradas.
 *   - Si n >= 1 imprime solo la línea n.
 */
void imprimir(Linea *cabeza, int n) {
    if (cabeza == NULL) {
        printf(COLOR_INFO "(el documento está vacío)\n" COLOR_RESET);
        return;
    }

    int numero = 1;
    Linea *actual = cabeza;
    while (actual != NULL) {
        if (n <= 0 || numero == n) {
            printf(COLOR_PARAM "%4d" COLOR_RESET " | %s\n", numero, actual->texto);
        }
        numero++;
        actual = actual->sig;
    }

    if (n >= 1 && n > numero - 1) {
        printf(COLOR_ERROR "No existe la línea %d (el documento tiene %d líneas).\n" COLOR_RESET,
               n, numero - 1);
    }
}

/**
 * Busca una palabra dentro de cada línea usando strstr (coincidencia de subcadena)
 * e imprime el número y el contenido de cada línea donde aparece.
 * Es una operación de SOLO LECTURA: no modifica la lista ni el disco.
 */
void buscar(Linea *cabeza, const char *palabra) {
    int numero = 1;
    int encontrados = 0;
    Linea *actual = cabeza;

    while (actual != NULL) {
        if (strstr(actual->texto, palabra) != NULL) {
            printf(COLOR_RESULT "%4d" COLOR_RESET " | %s\n", numero, actual->texto);
            encontrados++;
        }
        numero++;
        actual = actual->sig;
    }

    if (encontrados == 0) {
        printf(COLOR_INFO "No se encontró '%s' en ninguna línea.\n" COLOR_RESET, palabra);
    } else {
        printf(COLOR_INFO "Se encontró '%s' en %d línea(s).\n" COLOR_RESET, palabra, encontrados);
    }
}

/**
 * Libera TODA la lista enlazada (cada texto y cada nodo) y deja la cabeza en NULL.
 * Así no quedan fugas de memoria al salir del editor.
 */
void liberar_lista(Linea **cabeza) {
    Linea *actual = *cabeza;
    while (actual != NULL) {
        Linea *siguiente = actual->sig; /* guardar el siguiente antes de liberar */
        free(actual->texto);
        free(actual);
        actual = siguiente;
    }
    *cabeza = NULL;
}

/* ====================================================================================
 * SINCRONIZACIÓN CON EL DISCO (lseek + write + ftruncate)
 * ==================================================================================== */

/**
 * Escribe TODO el contenido de la lista en el archivo abierto (descriptor fd).
 *
 * Pasos (y por qué):
 *   1. lseek(fd, 0, SEEK_SET): mover el "cursor" del archivo al inicio para
 *      sobrescribir desde el principio.
 *   2. write(): escribir cada línea seguida de un '\n', contando los bytes totales.
 *   3. ftruncate(fd, total): recortar el archivo al tamaño exacto escrito. Esto es
 *      indispensable cuando el documento se hizo MÁS CORTO (por ejemplo tras un 'd'),
 *      porque si no, quedarían bytes viejos "colgando" al final del archivo.
 */
void guardar_en_disco(int fd, Linea *cabeza) {
    if (fd < 0) {
        return; /* No hay archivo abierto: nada que guardar */
    }

    /* 1. Posicionar el cursor del archivo al inicio */
    LOG_SYSCALL("lseek", "%d, 0, SEEK_SET", fd);
    off_t pos = lseek(fd, 0, SEEK_SET);
    if (pos == -1) {
        LOG_SYSCALL_ERROR("error");
        return;
    }
    LOG_SYSCALL_RESULT(pos);

    /* 2. Escribir cada línea + su salto de línea, acumulando el total de bytes */
    long total = 0;
    Linea *actual = cabeza;
    while (actual != NULL) {
        int largo = (int)strlen(actual->texto);

        LOG_SYSCALL("write", "%d, \"%s\", %d", fd, actual->texto, largo);
        ssize_t escritos = write(fd, actual->texto, largo);
        if (escritos == -1) {
            LOG_SYSCALL_ERROR("error");
            return;
        }
        LOG_SYSCALL_RESULT(escritos);
        total += escritos;

        /* Escribir el salto de línea que separa una línea de la siguiente */
        LOG_SYSCALL("write", "%d, \"\\n\", 1", fd);
        ssize_t salto = write(fd, "\n", 1);
        if (salto == -1) {
            LOG_SYSCALL_ERROR("error");
            return;
        }
        LOG_SYSCALL_RESULT(salto);
        total += salto;

        actual = actual->sig;
    }

    /* 3. Recortar el archivo al tamaño exacto escrito */
    LOG_SYSCALL("ftruncate", "%d, %ld", fd, total);
    if (ftruncate(fd, (off_t)total) == -1) {
        LOG_SYSCALL_ERROR("error");
        return;
    }
    LOG_SYSCALL_RESULT(0);
}

/* ====================================================================================
 * APERTURA Y CARGA DESDE EL DISCO (open + read)
 * ==================================================================================== */

/**
 * Abre (o crea) el archivo indicado y carga su contenido en una lista enlazada nueva.
 *
 * Parámetros:
 *   - nombre: ruta del archivo.
 *   - fd_salida: por aquí se devuelve el descriptor de archivo abierto.
 *
 * Devuelve la cabeza de la lista construida (puede ser NULL si el archivo está vacío).
 * Si open falla, deja *fd_salida en -1 y devuelve NULL.
 */
Linea *abrir_archivo(const char *nombre, int *fd_salida) {
    Linea *cabeza = NULL;

    /* Abrir en lectura/escritura; crear si no existe, con permisos 0644 (rw-r--r--) */
    LOG_SYSCALL("open", "\"%s\", O_RDWR|O_CREAT, 0644", nombre);
    int fd = open(nombre, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        LOG_SYSCALL_ERROR("error");
        *fd_salida = -1;
        return NULL;
    }
    LOG_SYSCALL_RESULT(fd);
    *fd_salida = fd;

    /*
     * Leer el archivo por bloques hacia un buffer dinámico que va creciendo.
     * Usamos un buffer temporal fijo y lo vamos agregando al buffer total con realloc.
     */
    char temporal[1024];
    char *contenido = NULL;   /* buffer dinámico con todo el archivo */
    long tamano = 0;          /* bytes acumulados en 'contenido' */
    ssize_t leidos;

    while (1) {
        /* Mostramos la syscall ANTES de ejecutarla y su resultado después,
           incluyendo la lectura final que devuelve 0 (fin de archivo, EOF). */
        LOG_SYSCALL("read", "%d, buffer, %d", fd, (int)sizeof(temporal));
        leidos = read(fd, temporal, sizeof(temporal));
        if (leidos == -1) {
            LOG_SYSCALL_ERROR("error");
            break;
        }
        LOG_SYSCALL_RESULT(leidos);

        if (leidos == 0) {
            break; /* EOF: ya no hay más bytes que leer */
        }

        char *mas_grande = (char *)realloc(contenido, tamano + leidos);
        if (mas_grande == NULL) {
            printf(COLOR_ERROR "Error: sin memoria al leer el archivo.\n" COLOR_RESET);
            free(contenido);
            return NULL; /* fd queda abierto; el llamador decidirá cerrarlo */
        }
        contenido = mas_grande;
        memcpy(contenido + tamano, temporal, leidos);
        tamano += leidos;
    }

    /*
     * Convertir el buffer de bytes en nodos de la lista, separando por '\n'.
     * Cada línea se copia a un pequeño buffer y se agrega con agregar_final.
     */
    long inicio = 0;
    for (long i = 0; i < tamano; i++) {
        if (contenido[i] == '\n') {
            long largo = i - inicio;
            char *linea = (char *)malloc(largo + 1);
            if (linea != NULL) {
                memcpy(linea, contenido + inicio, largo);
                linea[largo] = '\0';
                agregar_final(&cabeza, linea);
                free(linea);
            }
            inicio = i + 1;
        }
    }
    /* Si el archivo no termina en '\n', agregar la última línea pendiente */
    if (inicio < tamano) {
        long largo = tamano - inicio;
        char *linea = (char *)malloc(largo + 1);
        if (linea != NULL) {
            memcpy(linea, contenido + inicio, largo);
            linea[largo] = '\0';
            agregar_final(&cabeza, linea);
            free(linea);
        }
    }

    free(contenido);
    return cabeza;
}

