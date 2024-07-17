# Sudoku LSGA GPU 
El presente proyecto busca implementar un algoritmo genético para resolver Sudokus en GPU y comparar el rendimiento con su versión secuencial.

## Versión CPU
Se encuentra en `src/main.cpp` y para ejecutarlo
### Inicializar proyecto de CMake
make init

### Compilar la solución
make build

### Ejecutar 
./src/build/SudokuLSGACPU

### (opcional linux o mac) Limpiar el proyecto
make clean

## Versión GPU
Actualmente, no es ejecutable, pero los kernels se encuentran en `extern/OpenCL-Wrapper/src/kernel.cpp`