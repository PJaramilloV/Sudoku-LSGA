# Sudoku LSGA GPU 
El presente proyecto busca implementar un algoritmo genético para resolver Sudokus en GPU y comparar el rendimiento con su versión secuencial.

## Inicializar proyecto de CMake
Desde la carpeta base `Sudoku-LSGA`:

`make init`

## Versión CPU
Se encuentra en `src/main.cpp` y `src/member.cpp` y para ejecutarlo:

### Compilar
`make cpu`

### Ejecutar 
`./build/src/SudokuLSGACPU`

### (opcional linux o mac) Limpiar el proyecto
`make clean`

## Versión GPU (OpenCL)
Actualmente, no es ejecutable, pero los kernels se encuentran en `src/OpenCL-Wrapper/src/kernel.cpp`
### Compilar
`make cl`
