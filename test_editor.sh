#!/bin/bash
# ====================================================================
# Script de pruebas del editor de texto (proyecto Shell SO2026B)
# Compila el shell y ejecuta una sesion guionada del editor para
# demostrar los comandos: o, a, p, i, d, s, q.
# Al final muestra el archivo que quedo en disco para verificar.
# ====================================================================

# Si un comando falla, detener el script
set -e

# Ir a la carpeta donde esta este script
cd "$(dirname "$0")"

ARCHIVO_PRUEBA="prueba_editor.txt"

echo "========================================"
echo " 1) Compilando el proyecto con make"
echo "========================================"
# El Makefile lanza el shell al terminar de compilar; le mandamos un
# 'exit' para que se cierre solo y el script pueda continuar.
echo "exit" | make

echo ""
echo "========================================"
echo " 2) Ejecutando sesion de prueba"
echo "========================================"
# Empezamos desde un archivo limpio
rm -f "$ARCHIVO_PRUEBA"

# Le pasamos los comandos al shell por la entrada estandar.
# 'editor' abre el editor; dentro usamos sus comandos; 'q' sale del
# editor y 'exit' cierra el shell.
./eafitOS <<FIN
editor $ARCHIVO_PRUEBA
a Primera linea
a Segunda linea
a Tercera linea
i 2 Linea insertada en la posicion 2
p
s linea
d 1
p
q
exit
FIN

echo ""
echo "========================================"
echo " 3) Contenido final de $ARCHIVO_PRUEBA"
echo "========================================"
cat -n "$ARCHIVO_PRUEBA"

echo ""
echo "Prueba terminada."
