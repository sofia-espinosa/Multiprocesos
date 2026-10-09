//Hecho por: Sofia Espinosa y Benjamin Garrido
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define DIM 1024 //Dimensión de cada vector (1024 características)
#define K 10     //Cantidad de vecinos más cercanos a buscar (Top-K)

//Estructura para almacenar un vecino de forma ordenada
typedef struct
{
    int indice;       //Posición original del vector en la base de datos global
    double distancia; //Distancia euclidiana calculada con respecto a la consulta
} Vecino;

/* Calcula la distancia euclidiana entre dos vectores de dimensión DIM
Se utiliza double para la suma acumulada para evitar errores de precisión */
double distancia_euclidiana(float *vector1, float *vector2)
{
    double suma = 0.0;
    for(int i = 0; i < DIM; i++)
    {
        double diferencia = vector1[i] - vector2[i];
        suma += diferencia * diferencia;
    }
    return sqrt(suma);
}

//Inserta un vecino dentro del arreglo local/global manteniendo un orden ascendente
void insertar_vecino(Vecino mejores[], int indice, double distancia)
{
    int posicion = -1;
    
    //Buscar la posición correcta donde debe ir el nuevo vecino
    for(int i = 0; i < K; i++)
    {
        if(distancia < mejores[i].distancia)
        {
            posicion = i;
            break;
        }
    }

    //Si la distancia es mayor que el peor de nuestros K vecinos actuales, se descarta
    if(posicion == -1)
        return;

    //Desplazar los elementos hacia la derecha para abrir espacio
    for(int i = K - 1; i > posicion; i--)
    {
        mejores[i] = mejores[i - 1];
    }

    //Insertar el nuevo vecino en la posición correcta
    mejores[posicion].indice = indice;
    mejores[posicion].distancia = distancia;
}

//Ordenamiento burbuja simple (Ordenado de menor a mayor)
void ordenar_vecinos(Vecino vecinos[], int cantidad)
{
    for(int i = 0; i < cantidad - 1; i++)
    {
        for(int j = i + 1; j < cantidad; j++)
        {
            if(vecinos[j].distancia < vecinos[i].distancia)
            {
                Vecino aux = vecinos[i];
                vecinos[i] = vecinos[j];
                vecinos[j] = aux;
            }
        }
    }
}

//Función principal
int main(int argc, char *argv[])
{
    int id_proceso;        //Identificador del proceso actual
    int cantidad_procesos; //Número total de procesos distribuidos

    //Inicialización del entorno MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_proceso);
    MPI_Comm_size(MPI_COMM_WORLD, &cantidad_procesos);

    int N = 0; //Cantidad total de vectores en la base de datos
    int Q = 0; //Cantidad total de consultas a resolver

    //Punteros para la memoria global
    float *base_datos = NULL;
    float *consultas = NULL;

    //El proceso 0 lee el archivo de entrada (Lectura Centralizada)
    if(id_proceso == 0)
    {
        //Leer cantidad de vectores
        if(scanf("%d", &N) != 1) {
            fprintf(stderr, "Error al leer N\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        //Asignación de memoria dinámica para la base de datos completa de forma lineal
        base_datos = (float *)malloc((long long)N * DIM * sizeof(float));

        for(int i = 0; i < N; i++)
        {
            for(int j = 0; j < DIM; j++)
            {
                if(scanf("%f", &base_datos[i * DIM + j]) != 1) {
                    fprintf(stderr, "Error al leer base_datos[%d][%d]\n", i, j);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
            }
        }

        //Leer cantidad de consultas
        if(scanf("%d", &Q) != 1) {
            fprintf(stderr, "Error al leer Q\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        //Asignación de memoria para todas las consultas de forma lineal
        consultas = (float *)malloc((long long)Q * DIM * sizeof(float));

        for(int i = 0; i < Q; i++)
        {
            for(int j = 0; j < DIM; j++)
            {
                if(scanf("%f", &consultas[i * DIM + j]) != 1) {
                    fprintf(stderr, "Error al leer consultas[%d][%d]\n", i, j);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
            }
        }
    }

    //El proceso 0 difunde N y Q a todos los demás nodos para que sepan cuánta
    //memoria calcular y cuántas iteraciones realizar
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&Q, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //Arreglos para definir cuántos elementos recibe cada proceso y desde qué posición
    int *cantidades = (int *)malloc(cantidad_procesos * sizeof(int));
    int *desplazamientos = (int *)malloc(cantidad_procesos * sizeof(int));

    int elementos_base = N / cantidad_procesos; //División equitativa básica
    int sobrantes = N % cantidad_procesos;      //Residuos si N no es divisible exacto

    int desplazamiento_actual = 0;

    //Planificación de la distribución
    for(int i = 0; i < cantidad_procesos; i++)
    {
        int elementos_locales = elementos_base;

        //Distribución equitativa del residuo: los primeros "sobrantes" procesos reciben 1 vector más
        if(i < sobrantes)
            elementos_locales++;

        //Multiplicamos por DIM porque MPI enviará floats individuales, no vectores completos
        cantidades[i] = elementos_locales * DIM;
        desplazamientos[i] = desplazamiento_actual;

        desplazamiento_actual += cantidades[i];
    }

    //Determinar cuántos vectores le corresponden específicamente al proceso
    int elementos_locales = cantidades[id_proceso] / DIM;

    //Alojar memoria para la porción local de la base de datos
    float *base_local = (float *)malloc((long long)elementos_locales * DIM * sizeof(float));

    //Distribuir de forma segura los vectores a cada nodo
    MPI_Scatterv(
        base_datos, cantidades, desplazamientos, MPI_FLOAT,
        base_local, cantidades[id_proceso], MPI_FLOAT,
        0, MPI_COMM_WORLD
    );

    //Buffer local para almacenar la consulta que se esté procesando en el ciclo actual
    float *consulta_actual = (float *)malloc(DIM * sizeof(float));

    //Estructuras locales para almacenar los top-K de este nodo
    Vecino mejores_locales[K];
    
    int indices_locales[K];
    double distancias_locales[K];

    //Arreglos lineales de recolección global (Solo asignados y utilizados por el proceso 0)
    int *todos_los_indices = NULL;
    double *todas_las_distancias = NULL;

    if(id_proceso == 0)
    {
        //El proceso 0 reserva espacio para recibir el Top-K de cada uno de los procesos
        todos_los_indices = (int *)malloc(K * cantidad_procesos * sizeof(int));
        todas_las_distancias = (double *)malloc(K * cantidad_procesos * sizeof(double));
    }

    //Resuelva todas las consultas (Distribución y Cómputo en Paralelo)
    for(int consulta = 0; consulta < Q; consulta++)
    {
        //El proceso 0 extrae la consulta correspondiente del bloque completo
        if(id_proceso == 0)
        {
            for(int i = 0; i < DIM; i++)
            {
                consulta_actual[i] = consultas[consulta * DIM + i];
            }
        }

        //Se envía de forma masiva la consulta actual a todos los procesos trabajadores
        MPI_Bcast(consulta_actual, DIM, MPI_FLOAT, 0, MPI_COMM_WORLD);

        //Inicializar los contenedores de mejores vecinos locales con distancias infinitas
        for(int i = 0; i < K; i++)
        {
            mejores_locales[i].indice = -1;
            mejores_locales[i].distancia = 1e100;
        }

        //Determinar el primer índice global de la base de datos asignado a este proceso
        int indice_global_inicial = desplazamientos[id_proceso] / DIM;

        //Cada proceso busca en sus propios datos locales
        for(int i = 0; i < elementos_locales; i++)
        {
            double distancia = distancia_euclidiana(
                &base_local[i * DIM],
                consulta_actual
            );

            //Intenta insertar el resultado en el top-K local
            insertar_vecino(
                mejores_locales,
                indice_global_inicial + i,
                distancia
            );
        }

        //Pasar los mejores resultados locales a vectores lineales nativos para MPI
        for(int i = 0; i < K; i++)
        {
            indices_locales[i] = mejores_locales[i].indice;
            distancias_locales[i] = mejores_locales[i].distancia;
        }
  
        //Recolectar todos los arreglos de índices en el proceso 0
        MPI_Gather(
            indices_locales, K, MPI_INT,
            todos_los_indices, K, MPI_INT,
            0, MPI_COMM_WORLD
        );

        //Recolectar todos los arreglos de distancias en el proceso 0
        MPI_Gather(
            distancias_locales, K, MPI_DOUBLE,
            todas_las_distancias, K, MPI_DOUBLE,
            0, MPI_COMM_WORLD
        );

        //El proceso 0 unifica los datos recibidos y genera el top-k global ordenado
        if(id_proceso == 0)
        {
            Vecino mejores_globales[K];

            //Inicializar el arreglo de mejores globales con distancias máximas
            for(int i = 0; i < K; i++)
            {
                mejores_globales[i].indice = -1;
                mejores_globales[i].distancia = 1e100;
            }

            //Unificar los mejores resultados de todos los procesos
            for(int i = 0; i < K * cantidad_procesos; i++)
            {
                if(todos_los_indices[i] != -1)
                {
                    insertar_vecino(
                        mejores_globales,
                        todos_los_indices[i],
                        todas_las_distancias[i]
                    );
                }
            }

            //Ordenar el Top-K global de menor a mayor distancia
            ordenar_vecinos(mejores_globales, K);

            //Imprimir el resultado de la consulta actual
            for(int i = 0; i < K; i++)
            {
                printf("%d %.6f\n",
                       mejores_globales[i].indice,
                       mejores_globales[i].distancia);
            }
        }
    }

    //Todos los procesos liberan sus memorias
    free(base_local);
    free(consulta_actual);
    free(cantidades);
    free(desplazamientos);
    
    //Proceso 0 limpia los buffers globales grandes
    if(id_proceso == 0)
    {
        free(base_datos);
        free(consultas);
        free(todos_los_indices);
        free(todas_las_distancias);
    }
    
    MPI_Finalize();
    return 0;
}