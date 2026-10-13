#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cmath>

using namespace std;

// Tipos de Procesamiento (Enum 'proceso' del enunciado)
const int COMIDA = 0;
const int ROPA = 1;
const int MUEBLE = 2;
const int ELECTRONICO = 3;
const int PROCESO_DESCONOCIDO = -1;

const int RAPIDO = 0;
const int SECUENCIAL = 1;
const int PARALELO = 2;
const int ESQUEMA_DESCONOCIDO = -1;

// Estados del Paquete (Enum 'EstadoPaquete' del enunciado)
const int ESPERANDO_EN_CENTRAL = 0;
const int EN_TRANSITO_A_PROC = 1;
const int EN_PROCESAMIENTO = 2;
const int LISTO_PARA_ENTREGA = 3;
const int EN_TRANSITO_A_DESTINO = 4;
const int ENTREGADO = 5;
const int RETRASADO = 6;

// Estados del Repartidor (Enum 'EstadoRepartidor' del enunciado)
const int LIBRE = 0;
const int OCUPADO = 1;

// =======================================================
// MODELOS DE DATOS DEL PROYECTO
// =======================================================

struct Coord {
    int x;
    int y;
};

struct Paquete {
    int id;
    int centralId;
    int horaInicial;
    int tipoProc;
    Coord direcEntrega;
    int horaMaxEntrega;
    
    int estadoActual;
    int tiempoRestanteProc;
    int centroAsignadoId;

    // Campos de auditoría y trazabilidad para el reporte final
    int idRepartidorPrimeraMilla;
    int tiempoAsignacionPrimeraMilla;
    int idRepartidorUltimaMilla;
    int tiempoAsignacionUltimaMilla;
    
    // Control de tiempos de viaje
    int tiempoFinTransitoProc;
    int tiempoFinTransitoDestino;

    Paquete() : id(0), centralId(0), horaInicial(0), tipoProc(COMIDA),
                horaMaxEntrega(0), estadoActual(ESPERANDO_EN_CENTRAL),
                tiempoRestanteProc(0), centroAsignadoId(-1),
                idRepartidorPrimeraMilla(-1), tiempoAsignacionPrimeraMilla(-1),
                idRepartidorUltimaMilla(-1), tiempoAsignacionUltimaMilla(-1),
                tiempoFinTransitoProc(-1), tiempoFinTransitoDestino(-1) {
        direcEntrega.x = 0;
        direcEntrega.y = 0;
    }
};

struct Central {
    int id;
    Coord direccion;
};

struct Repartidor {
    int id;
    Coord posicion;
    int estado;
    Paquete* paqueteActual;
    Coord direccionObjetivoActual;
};

// =======================================================
// ESTRUCTURAS DINÁMICAS SIN USO DE STL
// =======================================================

template <typename T>
struct Nodo {
    T dato;
    Nodo<T>* siguiente;
    Nodo(T valor) : dato(valor), siguiente(nullptr) {}
};

template <typename T>
class Lista {
private:
    Nodo<T>* cabeza;
    Nodo<T>* cola;
    int tamano;

public:
    Lista() : cabeza(nullptr), cola(nullptr), tamano(0) {}

    ~Lista() {
        limpiar();
    }

    void limpiar() {
        Nodo<T>* actual = cabeza;
        while (actual != nullptr) {
            Nodo<T>* sig = actual->siguiente;
            delete actual;
            actual = sig;
        }
        cabeza = nullptr;
        cola = nullptr;
        tamano = 0;
    }

    void agregarFinal(T valor) {
        Nodo<T>* nuevo = new Nodo<T>(valor);
        if (cabeza == nullptr) {
            cabeza = nuevo;
            cola = nuevo;
        } else {
            cola->siguiente = nuevo;
            cola = nuevo;
        }
        tamano++;
    }

    int getTamano() const { return tamano; }
    Nodo<T>* getCabeza() const { return cabeza; }
};

template <typename T>
class Cola {
private:
    Nodo<T>* frente;
    Nodo<T>* final;
    int tamano;

public:
    Cola() : frente(nullptr), final(nullptr), tamano(0) {}

    ~Cola() {
        while (!estaVacia()) desencolar();
    }

    bool estaVacia() const { return frente == nullptr; }
    int getTamano() const { return tamano; }

    void encolar(T valor) {
        Nodo<T>* nuevo = new Nodo<T>(valor);
        if (estaVacia()) {
            frente = final = nuevo;
        } else {
            final->siguiente = nuevo;
            final = nuevo;
        }
        tamano++;
    }

    T desencolar() {
        if (estaVacia()) exit(1);
        Nodo<T>* temp = frente;
        T valor = temp->dato;
        frente = frente->siguiente;
        if (frente == nullptr) final = nullptr;
        delete temp;
        tamano--;
        return valor;
    }

    T obtenerFrente() const {
        if (estaVacia()) exit(1);
        return frente->dato;
    }
};

template <typename T>
class Pila {
private:
    Nodo<T>* tope;
    int tamano;

public:
    Pila() : tope(nullptr), tamano(0) {}

    ~Pila() {
        while (!estaVacia()) desapilar();
    }

    bool estaVacia() const { return tope == nullptr; }
    int getTamano() const { return tamano; }

    void apilar(T valor) {
        Nodo<T>* nuevo = new Nodo<T>(valor);
        nuevo->siguiente = tope;
        tope = nuevo;
        tamano++;
    }

    T desapilar() {
        if (estaVacia()) exit(1);
        Nodo<T>* temp = tope;
        T valor = temp->dato;
        tope = tope->siguiente;
        delete temp;
        tamano--;
        return valor;
    }

    T obtenerTope() const {
        if (estaVacia()) exit(1);
        return tope->dato;
    }
};

// Modelo unificado de Centro de Procesamiento con sus estructuras de gestión
struct CentroProcesamiento {
    int id;
    Coord direccion;
    int tipoProcesamiento;
    int esquema;
    int tiempoProcBase;
    int capacidadMaxima;
    int ocupacionActual;

    // Colas, pilas y listas internas según el esquema operativo
    Cola<Paquete*> colaSecuencial;
    Pila<Paquete*> pilaRapida;
    Lista<Paquete*> listaParalela;
};

// =======================================================
// FUNCIONES AUXILIARES Y GEOMETRÍA
// =======================================================

int convertirTipoProceso(const string& texto) {
    if (texto == "COMIDA") return COMIDA;
    if (texto == "ROPA") return ROPA;
    if (texto == "MUEBLE") return MUEBLE;
    if (texto == "ELECTRONICO") return ELECTRONICO;
    return PROCESO_DESCONOCIDO;
}

int convertirEsquema(const string& texto) {
    if (texto == "RAPIDO") return RAPIDO;
    if (texto == "SECUENCIAL") return SECUENCIAL;
    if (texto == "PARALELO") return PARALELO;
    return ESQUEMA_DESCONOCIDO;
}

// Distancia euclidiana truncada al entero inferior
int calcularDistancia(Coord c1, Coord c2) {
    double dx = c1.x - c2.x;
    double dy = c1.y - c2.y;
    double dist = sqrt(dx * dx + dy * dy);
    return (int)floor(dist);
}

// =======================================================
// LECTURA DE ARCHIVOS DE ENTRADA
// =======================================================

bool leerPedidos(const string& ruta, Lista<Paquete*>& listaPaquetes) {
    ifstream archivo(ruta);
    if (!archivo.is_open()) return false;
    
    int id, centralId, horaInit, horaMax, x, y;
    string textoProc;
    while (archivo >> id >> centralId >> horaInit >> textoProc >> x >> y >> horaMax) {
        Paquete* p = new Paquete();
        p->id = id;
        p->centralId = centralId;
        p->horaInicial = horaInit;
        p->tipoProc = convertirTipoProceso(textoProc);
        p->direcEntrega.x = x;
        p->direcEntrega.y = y;
        p->horaMaxEntrega = horaMax;
        listaPaquetes.agregarFinal(p);
    }
    archivo.close();
    return true;
}

bool leerCentrales(const string& ruta, Lista<Central>& listaCentrales) {
    ifstream archivo(ruta);
    if (!archivo.is_open()) return false;

    Central c;
    while (archivo >> c.id >> c.direccion.x >> c.direccion.y) {
        listaCentrales.agregarFinal(c);
    }
    archivo.close();
    return true;
}

bool leerCentros(const string& ruta, Lista<CentroProcesamiento*>& listaCentros) {
    ifstream archivo(ruta);
    if (!archivo.is_open()) return false;

    int id, x, y, tBase, capMax;
    string textoProc, textoEsq;
    while (archivo >> id >> x >> y >> textoProc >> textoEsq >> tBase >> capMax) {
        CentroProcesamiento* centro = new CentroProcesamiento();
        centro->id = id;
        centro->direccion.x = x;
        centro->direccion.y = y;
        centro->tipoProcesamiento = convertirTipoProceso(textoProc);
        centro->esquema = convertirEsquema(textoEsq);
        centro->tiempoProcBase = tBase;
        centro->capacidadMaxima = capMax;
        centro->ocupacionActual = 0;
        listaCentros.agregarFinal(centro);
    }
    archivo.close();
    return true;
}

bool leerRepartidores(const string& ruta, Lista<Repartidor*>& listaRepartidores) {
    ifstream archivo(ruta);
    if (!archivo.is_open()) return false;

    int id, x, y;
    while (archivo >> id >> x >> y) {
        Repartidor* rep = new Repartidor();
        rep->id = id;
        rep->posicion.x = x;
        rep->posicion.y = y;
        rep->estado = LIBRE;
        rep->paqueteActual = nullptr;
        rep->direccionObjetivoActual = rep->posicion;
        listaRepartidores.agregarFinal(rep);
    }
    archivo.close();
    return true;
}

// Búsqueda de elementos por ID
Central* buscarCentral(Lista<Central>& listaCentrales, int idBuscado) {
    Nodo<Central>* nodoActual = listaCentrales.getCabeza();
    while (nodoActual != nullptr) {
        if (nodoActual->dato.id == idBuscado) return &(nodoActual->dato);
        nodoActual = nodoActual->siguiente;
    }
    return nullptr;
}

CentroProcesamiento* buscarCentro(Lista<CentroProcesamiento*>& listaCentros, int idBuscado) {
    Nodo<CentroProcesamiento*>* nodoActual = listaCentros.getCabeza();
    while (nodoActual != nullptr) {
        if (nodoActual->dato->id == idBuscado) return nodoActual->dato;
        nodoActual = nodoActual->siguiente;
    }
    return nullptr;
}

// =======================================================
// SIMULACIÓN DEL CICLO DE TRÁNSITO LOCAL (T = 0, 1, 2...)
// =======================================================

void simularLogistica(
    Lista<Paquete*>& paquetes,
    Lista<Central>& centrales,
    Lista<CentroProcesamiento*>& centros,
    Lista<Repartidor*>& repartidores
) {
    int tiempo = 0;
    int totalPaquetes = paquetes.getTamano();
    int paquetesFinalizados = 0;

    while (paquetesFinalizados < totalPaquetes) {
        
        // ---------------------------------------------------
        // FASE 1: Finalización de tránsitos de llegada
        // ---------------------------------------------------
        Nodo<Paquete*>* nodoPaquete = paquetes.getCabeza();
        while (nodoPaquete != nullptr) {
            Paquete* p = nodoPaquete->dato;
            
            // Llegada al Centro de Procesamiento
            if (p->estadoActual == EN_TRANSITO_A_PROC && tiempo == p->tiempoFinTransitoProc) {
                p->estadoActual = EN_PROCESAMIENTO;
                CentroProcesamiento* centro = buscarCentro(centros, p->centroAsignadoId);
                p->tiempoRestanteProc = centro->tiempoProcBase;
                
                // Liberar repartidor de Primera Milla en la posición del centro
                Nodo<Repartidor*>* nodoRep = repartidores.getCabeza();
                while (nodoRep != nullptr) {
                    if (nodoRep->dato->id == p->idRepartidorPrimeraMilla && nodoRep->dato->estado == OCUPADO) {
                        nodoRep->dato->estado = LIBRE;
                        nodoRep->dato->posicion = centro->direccion;
                        nodoRep->dato->paqueteActual = nullptr;
                        break;
                    }
                    nodoRep = nodoRep->siguiente;
                }

                // Encolar o apilar según esquema
                if (centro->esquema == SECUENCIAL) {
                    centro->colaSecuencial.encolar(p);
                } else if (centro->esquema == RAPIDO) {
                    centro->pilaRapida.apilar(p);
                } else if (centro->esquema == PARALELO) {
                    centro->listaParalela.agregarFinal(p);
                }
            }
            // Llegada al Destino Final
            else if (p->estadoActual == EN_TRANSITO_A_DESTINO && tiempo == p->tiempoFinTransitoDestino) {
                if (tiempo <= p->horaMaxEntrega) {
                    p->estadoActual = ENTREGADO;
                } else {
                    p->estadoActual = RETRASADO;
                }
                paquetesFinalizados++;

                // Liberar repartidor de Última Milla en la ubicación del comprador
                Nodo<Repartidor*>* nodoRep = repartidores.getCabeza();
                while (nodoRep != nullptr) {
                    if (nodoRep->dato->id == p->idRepartidorUltimaMilla && nodoRep->dato->estado == OCUPADO) {
                        nodoRep->dato->estado = LIBRE;
                        nodoRep->dato->posicion = p->direcEntrega;
                        nodoRep->dato->paqueteActual = nullptr;
                        break;
                    }
                    nodoRep = nodoRep->siguiente;
                }

                CentroProcesamiento* centro = buscarCentro(centros, p->centroAsignadoId);
                if (centro->ocupacionActual > 0) centro->ocupacionActual--;
            }
            nodoPaquete = nodoPaquete->siguiente;
        }

        // ---------------------------------------------------
        // FASE 2: Asignación de Última Milla (Despacho)
        // ---------------------------------------------------
        nodoPaquete = paquetes.getCabeza();
        while (nodoPaquete != nullptr) {
            Paquete* p = nodoPaquete->dato;
            if (p->estadoActual == LISTO_PARA_ENTREGA) {
                CentroProcesamiento* centro = buscarCentro(centros, p->centroAsignadoId);

                Repartidor* mejorRepartidor = nullptr;
                int menorTiempoTotal = 1e9;

                Nodo<Repartidor*>* nodoRep = repartidores.getCabeza();
                while (nodoRep != nullptr) {
                    Repartidor* rep = nodoRep->dato;
                    if (rep->estado == LIBRE) {
                        int tLlegadaPlanta = calcularDistancia(rep->posicion, centro->direccion);
                        int tViajeDestino = calcularDistancia(centro->direccion, p->direcEntrega);
                        int tTotal = tLlegadaPlanta + tViajeDestino;

                        if (tTotal < menorTiempoTotal) {
                            menorTiempoTotal = tTotal;
                            mejorRepartidor = rep;
                        } else if (tTotal == menorTiempoTotal && mejorRepartidor != nullptr) {
                            if (rep->id < mejorRepartidor->id) {
                                mejorRepartidor = rep;
                            }
                        }
                    }
                    nodoRep = nodoRep->siguiente;
                }

                if (mejorRepartidor != nullptr) {
                    p->estadoActual = EN_TRANSITO_A_DESTINO;
                    p->idRepartidorUltimaMilla = mejorRepartidor->id;
                    p->tiempoAsignacionUltimaMilla = tiempo;

                    mejorRepartidor->estado = OCUPADO;
                    mejorRepartidor->paqueteActual = p;

                    p->tiempoFinTransitoDestino = tiempo + menorTiempoTotal;
                }
            }
            nodoPaquete = nodoPaquete->siguiente;
        }

        // ---------------------------------------------------
        // FASE 3: Procesamiento interno en Centros
        // ---------------------------------------------------
        Nodo<CentroProcesamiento*>* nodoCentro = centros.getCabeza();
        while (nodoCentro != nullptr) {
            CentroProcesamiento* centro = nodoCentro->dato;

            if (centro->esquema == SECUENCIAL && !centro->colaSecuencial.estaVacia()) {
                Paquete* p = centro->colaSecuencial.obtenerFrente();
                p->tiempoRestanteProc--;
                if (p->tiempoRestanteProc == 0) {
                    centro->colaSecuencial.desencolar();
                    p->estadoActual = LISTO_PARA_ENTREGA;
                }
            } 
            else if (centro->esquema == RAPIDO && !centro->pilaRapida.estaVacia()) {
                Paquete* p = centro->pilaRapida.obtenerTope();
                p->tiempoRestanteProc--;
                if (p->tiempoRestanteProc == 0) {
                    centro->pilaRapida.desapilar();
                    p->estadoActual = LISTO_PARA_ENTREGA;
                }
            } 
            else if (centro->esquema == PARALELO && centro->listaParalela.getTamano() > 0) {
                Nodo<Paquete*>* subNodo = centro->listaParalela.getCabeza();
                while (subNodo != nullptr) {
                    Paquete* p = subNodo->dato;
                    if (p->estadoActual == EN_PROCESAMIENTO) {
                        p->tiempoRestanteProc--;
                        if (p->tiempoRestanteProc == 0) {
                            p->estadoActual = LISTO_PARA_ENTREGA;
                        }
                    }
                    subNodo = subNodo->siguiente;
                }
            }
            nodoCentro = nodoCentro->siguiente;
        }

        // ---------------------------------------------------
        // FASE 4: Asignación de Primera Milla (Recepción)
        // ---------------------------------------------------
        nodoPaquete = paquetes.getCabeza();
        while (nodoPaquete != nullptr) {
            Paquete* p = nodoPaquete->dato;
            if (p->estadoActual == ESPERANDO_EN_CENTRAL && p->horaInicial <= tiempo) {
                Central* centralPtr = buscarCentral(centrales, p->centralId);
                
                CentroProcesamiento* mejorCentro = nullptr;
                int menorTiempoLiberacion = 1e9;

                Nodo<CentroProcesamiento*>* iterCentro = centros.getCabeza();
                while (iterCentro != nullptr) {
                    CentroProcesamiento* centro = iterCentro->dato;
                    if (centro->tipoProcesamiento == p->tipoProc && centro->ocupacionActual < centro->capacidadMaxima) {
                        int distTraslado = calcularDistancia(centralPtr->direccion, centro->direccion);
                        int tLiberacion = distTraslado + centro->tiempoProcBase;
                        
                        if (tLiberacion < menorTiempoLiberacion) {
                            menorTiempoLiberacion = tLiberacion;
                            mejorCentro = centro;
                        } else if (tLiberacion == menorTiempoLiberacion && mejorCentro != nullptr) {
                            if (centro->id < mejorCentro->id) {
                                mejorCentro = centro;
                            }
                        }
                    }
                    iterCentro = iterCentro->siguiente;
                }

                if (mejorCentro != nullptr) {
                    Repartidor* mejorRepartidor = nullptr;
                    int menorDistanciaRep = 1e9;

                    Nodo<Repartidor*>* iterRep = repartidores.getCabeza();
                    while (iterRep != nullptr) {
                        Repartidor* rep = iterRep->dato;
                        if (rep->estado == LIBRE) {
                            int dist = calcularDistancia(rep->posicion, centralPtr->direccion);
                            if (dist < menorDistanciaRep) {
                                menorDistanciaRep = dist;
                                mejorRepartidor = rep;
                            } else if (dist == menorDistanciaRep && mejorRepartidor != nullptr) {
                                if (rep->id < mejorRepartidor->id) {
                                    mejorRepartidor = rep;
                                }
                            }
                        }
                        iterRep = iterRep->siguiente;
                    }

                    if (mejorRepartidor != nullptr) {
                        p->estadoActual = EN_TRANSITO_A_PROC;
                        p->centroAsignadoId = mejorCentro->id;
                        p->idRepartidorPrimeraMilla = mejorRepartidor->id;
                        p->tiempoAsignacionPrimeraMilla = tiempo;

                        mejorRepartidor->estado = OCUPADO;
                        mejorRepartidor->paqueteActual = p;

                        int dist1 = calcularDistancia(mejorRepartidor->posicion, centralPtr->direccion);
                        int dist2 = calcularDistancia(centralPtr->direccion, mejorCentro->direccion);
                        
                        p->tiempoFinTransitoProc = tiempo + dist1 + dist2;
                        mejorCentro->ocupacionActual++;
                    }
                }
            }
            nodoPaquete = nodoPaquete->siguiente;
        }

        tiempo++;
    }
}

// =======================================================
// IMPRESIÓN DEL REPORTE FINAL DE SALIDA
// =======================================================

void imprimirReporte(Lista<Paquete*>& paquetes) {
    Nodo<Paquete*>* nodoActual = paquetes.getCabeza();
    while (nodoActual != nullptr) {
        Paquete* p = nodoActual->dato;
        string estadoTexto = (p->estadoActual == ENTREGADO) ? "ENTREGADO" : "RETRASADO";
        
        cout << p->id << " "
             << estadoTexto << " "
             << p->idRepartidorPrimeraMilla << " "
             << p->tiempoAsignacionPrimeraMilla << " "
             << p->idRepartidorUltimaMilla << " "
             << p->tiempoAsignacionUltimaMilla << "\n";
             
        nodoActual = nodoActual->siguiente;
    }
}

// Liberación explícita de memoria dinámica en heap
void liberarMemoria(
    Lista<Paquete*>& paquetes,
    Lista<CentroProcesamiento*>& centros,
    Lista<Repartidor*>& repartidores
) {
    Nodo<Paquete*>* nodoP = paquetes.getCabeza();
    while (nodoP != nullptr) {
        delete nodoP->dato;
        nodoP = nodoP->siguiente;
    }

    Nodo<CentroProcesamiento*>* nodoC = centros.getCabeza();
    while (nodoC != nullptr) {
        delete nodoC->dato;
        nodoC = nodoC->siguiente;
    }

    Nodo<Repartidor*>* nodoR = repartidores.getCabeza();
    while (nodoR != nullptr) {
        delete nodoR->dato;
        nodoR = nodoR->siguiente;
    }
}

// =======================================================
// FUNCIÓN PRINCIPAL
// =======================================================

int main(int argc, char* argv[]) {
    if (argc < 6) {
        cerr << "Uso: " << argv[0] << " pedidos.in centrales.in procesamiento.in repartidores.in calles.in\n";
        return 1;
    }

    Lista<Paquete*> paquetes;
    Lista<Central> centrales;
    Lista<CentroProcesamiento*> centros;
    Lista<Repartidor*> repartidores;

    if (!leerPedidos(argv[1], paquetes) ||
        !leerCentrales(argv[2], centrales) ||
        !leerCentros(argv[3], centros) ||
        !leerRepartidores(argv[4], repartidores)) {
        cerr << "Error al abrir los archivos de entrada.\n";
        return 1;
    }

    simularLogistica(paquetes, centrales, centros, repartidores);
    imprimirReporte(paquetes);
    liberarMemoria(paquetes, centros, repartidores);

    return 0;
}
