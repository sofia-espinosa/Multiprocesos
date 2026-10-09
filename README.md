# Algoritmos Concurrencia y Búsqueda Paralela

Este repositorio contiene implementaciones de algoritmos optimizados para procesamiento concurrente y distribuido, desarrollados en **Python** y **C** empleando librerías de programación paralela de alto rendimiento (`multiprocessing` y `Open MPI`).

---

## 📋 Contenido del Repositorio

| Archivo | Lenguaje | Paradigma | Descripción |
| :--- | :--- | :--- | :--- |
| `Indice_invertido.py` | Python | Concurrencia (Multiprocessing) | Construcción concurrente de un índice invertido de documentos usando memoria compartida y cerrojos (`Lock`). |
| `trabajo3.c` | C | Computación Distribuida (MPI) | Búsqueda distribuida de los $K$ vecinos más cercanos ($K$-NN) sobre vectores de alta dimensión ($1024$ características). |

---

## 🛠️ Detalle de Implementación

### 1. Índice Invertido Concurrente (`Indice_invertido.py`)
- **Objetivo:** Indexar múltiples documentos y asociar términos con sus archivos fuente de manera simultánea.
- **Mecanismos usados:**
  - `multiprocessing.Process` para dividir la carga de trabajo entre $N$ procesos independientes.
  - `multiprocessing.Manager().dict()` para mantener el índice global en memoria compartida.
  - `multiprocessing.Lock()` para garantizar exclusión mutua durante las escrituras concurrentes.

### 2. Buscador Top-$K$ Vecinos Más Cercanos (`trabajo3.c`)
- **Objetivo:** Calcular la distancia euclidiana entre un conjunto de vectores de consulta $Q$ y una base de datos de $N$ vectores en un entorno distribuido.
- **Mecanismos usados:**
  - **Estructura de datos:** Matriz de dimensión $N \times 1024$.
  - **Distribución de datos:** `MPI_Scatterv` para repartir de manera equitativa el volumen de vectores a cada nodo de trabajo.
  - **Sincronización:** `MPI_Bcast` para difundir las consultas y `MPI_Gather` para consolidar el Top-$K$ parcial de cada nodo en el proceso maestro (Rank 0).

---
