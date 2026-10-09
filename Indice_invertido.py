import multiprocessing

def indice_invertido(pid, num_procesos, lista_documento, indice, cerrojo):
    for x in range(pid, len(lista_documento), num_procesos):
        documento = lista_documento[x]
        nom_archivo = documento["Nombre"]
        palabras = documento["Palabras"]

        for palabra in palabras:
            cerrojo.acquire()
            try:
                if palabra not in indice:
                    indice[palabra] = []
                lista_aux = indice[palabra]
                if nom_archivo not in lista_aux:
                    lista_aux.append(nom_archivo)
                    indice[palabra] = lista_aux
            finally:
                cerrojo.release()

if __name__ == "__main__":
    cantidad_procesos = 5

    n_documentos = int(input().strip())
    
    lista_documentos = []
    for x in range(n_documentos):
        nombre_archivo = input().strip()
        cantidad_terminos = int(input().strip())
        palabras = input().strip().split()
        lista_documentos.append({"Nombre" : nombre_archivo, "Palabras" :palabras })

    num_consultas = int(input().strip())
    consultas = input().strip().split()

    manager = multiprocessing.Manager()
    indice = manager.dict()
    cerrojo = multiprocessing.Lock()

    procesos = []

    for x in range(cantidad_procesos):
        p = multiprocessing.Process(
            target = indice_invertido, 
            args = (x, cantidad_procesos, lista_documentos, indice, cerrojo))
        procesos.append(p)
    
    for p in procesos:
        p.start()
    
    for p in procesos:
        p.join()

    for consulta in consultas:
        if consulta in indice:
            resultado = " ".join(indice[consulta])
            print(f'Resultados para "{consulta}": {resultado}')
        else:
            print(f'Resultados para "{consulta}": No hay resultados.')