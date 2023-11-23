#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <filesystem>
#include <utility>
#include <semaphore>
#include <condition_variable>

std::queue<std::pair<std::string, double>> colaGenomasSemaforo;
std::counting_semaphore<1> semaforo(1);

std::queue<std::pair<std::string, double>> colaGenomasMutex;
std::mutex mutexCola;
std::condition_variable condVar;


double calcularContenidoCGTotal(const std::string& rutaArchivo) {
    std::ifstream archivoGenoma(rutaArchivo);
    std::string linea;
    int conteoC = 0, conteoG = 0, total = 0;

    if (!archivoGenoma.is_open()) {
        std::cerr << "No se pudo abrir el archivo: " << rutaArchivo << std::endl;
        return 0.0;
    }

    while (std::getline(archivoGenoma, linea)) {
        if (!linea.empty() && linea[0] == '>') continue; // Ignorar líneas que no son secuencias

        for (char base : linea) {
            if (base == 'C') conteoC++;
            else if (base == 'G') conteoG++;
            if (base == 'A' || base == 'T' || base == 'C' || base == 'G') {
                total++;
            }
        }
    }

    archivoGenoma.close();
    return total > 0 ? static_cast<double>(conteoC + conteoG) / total : 0.0;
}

void agregarAColaConSemaforo(const std::string& nombreArchivo, double contenidoCG) {
    semaforo.acquire();
    colaGenomasSemaforo.push({nombreArchivo, contenidoCG});
    semaforo.release();
}

void agregarAColaConMutex(const std::string& nombreArchivo, double contenidoCG) {
    std::unique_lock<std::mutex> lock(mutexCola);
    colaGenomasMutex.push({nombreArchivo, contenidoCG});
    condVar.notify_one();
}



void procesarArchivoGenoma(const std::string& rutaArchivo, double umbralCG, int modo) {
    double contenidoCG = calcularContenidoCGTotal(rutaArchivo);

    if (contenidoCG > umbralCG) {
        if (modo == 1) {
            agregarAColaConSemaforo(rutaArchivo, contenidoCG);
        } else if (modo == 2) {
            agregarAColaConMutex(rutaArchivo, contenidoCG);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <directorio> <modo: 1 (Semaforos) | 2 (Mutex)>" << std::endl;
        return 1;
    }

    std::string directorioGenomas = argv[1];
    int modo = std::stoi(argv[2]);
    const double umbralCG = 0.60; //! Umbral del contenido de CG
    std::vector<std::thread> hilos;

    for (const auto& entrada : std::filesystem::directory_iterator(directorioGenomas)) {
        if (entrada.is_regular_file()) {
            hilos.emplace_back(procesarArchivoGenoma, entrada.path().string(), umbralCG, modo);
        }
    }

    for (auto& hilo : hilos) {
        hilo.join();
    }

    // Procesar la cola según el modo
    if (modo == 1) {
        while (!colaGenomasSemaforo.empty()) {
            auto& [nombreArchivo, contenidoCG] = colaGenomasSemaforo.front();
            std::cout << "Genoma: " << nombreArchivo << ", Contenido CG: " << contenidoCG << std::endl;
            colaGenomasSemaforo.pop();
        }
    } else if (modo == 2) {
        while (!colaGenomasMutex.empty()) {
            std::unique_lock<std::mutex> lock(mutexCola);
            auto& [nombreArchivo, contenidoCG] = colaGenomasMutex.front();
            std::cout << "Genoma: " << nombreArchivo << ", Contenido CG: " << contenidoCG << std::endl;
            colaGenomasMutex.pop();
            condVar.notify_one();
        }
    }

    return 0;
}