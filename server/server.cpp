// Servidor: espera a los 3 clientes, promedia sus 4 matrices y les devuelve las medias.
// Uso: ./server <puerto>
// No sabe nada del entrenamiento: atiende rondas sin fin hasta Ctrl+C.
//
// Protocolo (cabecera de texto, 18 caracteres) + payload binario:
//   TYPE(1) CLIENT_ID(1) ROWS(4) COLS(4) PAYLOAD_LEN(8) | PAYLOAD
//   M: cliente -> servidor, matriz de una capa (doubles crudos)
//   A: servidor -> cliente, media de una capa (doubles crudos)
//   E: servidor -> cliente, texto con el codigo de error

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>

#define NUM_CLIENTES 3
#define NUM_CAPAS 4

// Una matriz: dimensiones + valores fila por fila
struct Matriz {
    int rows, cols;
    std::vector<double> data;
};

ssize_t readExact(int fd, char *buf, size_t n) {
    size_t total = 0;
    while (total < n) {
        ssize_t r = read(fd, buf + total, n - total);
        if (r <= 0) return r;   
        total += (size_t)r;
    }
    return (ssize_t)total;
}

ssize_t writeExact(int fd, const char *buf, size_t n) {
    size_t total = 0;
    while (total < n) {
        ssize_t w = write(fd, buf + total, n - total);
        if (w <= 0) return w;
        total += (size_t)w;
    }
    return (ssize_t)total;
}

std::string zeroPad(int number, int size) {
    std::string str = std::to_string(number);
    if ((int)str.length() >= size) return str;
    return std::string(size - str.length(), '0') + str;
}

std::string armarCabecera(char type, int client_id, int rows, int cols, int payload_len) {
    return std::string(1, type) + std::to_string(client_id) + zeroPad(rows, 4) + zeroPad(cols, 4) + zeroPad(payload_len, 8);
}

int leerCampo(int S, int size) {
    char buff[16];
    if (readExact(S, buff, size) <= 0)
        throw std::runtime_error("Conexion cerrada por el cliente");
    buff[size] = '\0';
    return atoi(buff);
}

// Lee los 4 mensajes M de un cliente. Devuelve su CLIENT_ID.
// Si algo esta mal lanza una excepcion cuyo texto es el codigo de error.
int leer_matrices(int S, std::vector<Matriz> &capas) {
    int id = 0;
    for (int capa = 0; capa < NUM_CAPAS; capa++) {
        char type;
        if (readExact(S, &type, 1) <= 0)
            throw std::runtime_error("Conexion cerrada por el cliente");

        int client_id= leerCampo(S, 1);
        int rows  = leerCampo(S, 4);
        int cols = leerCampo(S, 4);
        int payload_len = leerCampo(S, 8);

        if (type != 'M' || payload_len != rows * cols * (int)sizeof(double))
            throw std::runtime_error("BAD_HEADER");
        if (client_id < 1 || client_id > NUM_CLIENTES || (capa > 0 && client_id != id))
            throw std::runtime_error("BAD_CLIENT");
        id = client_id;

        Matriz M;
        M.rows = rows;
        M.cols = cols;
        M.data.resize(rows * cols);
        if (readExact(S, (char *)M.data.data(), payload_len) <= 0)
            throw std::runtime_error("Conexion cerrada leyendo la matriz");

        capas.push_back(M);
    }
    return id;
}

// Media elemento por elemento: Media_k[i][j] = (C1_k[i][j] + C2_k[i][j] + C3_k[i][j]) / 3
std::vector<Matriz> calcular_medias(const std::vector<std::vector<Matriz>> &recibidas) {
    std::vector<Matriz> medias = recibidas[0];   // copia para tener las dimensiones
    for (int k = 0; k < NUM_CAPAS; k++) {
        for (size_t i = 0; i < medias[k].data.size(); i++) {
            double suma = 0;
            for (int c = 0; c < NUM_CLIENTES; c++)
                suma += recibidas[c][k].data[i];
            medias[k].data[i] = suma / NUM_CLIENTES;
        }
    }
    return medias;
}

// Envia las 4 medias como 4 mensajes A
void enviar_medias(int S, int client_id, const std::vector<Matriz> &medias) {
    for (const Matriz &M : medias) {
        int bytes = M.rows * M.cols * (int)sizeof(double);
        std::string cab = armarCabecera('A', client_id, M.rows, M.cols, bytes);
        writeExact(S, cab.c_str(), cab.size());
        writeExact(S, (const char *)M.data.data(), bytes);
    }
}

void enviar_error(int S, const std::string &codigo) {
    std::string cab = armarCabecera('E', 0, 0, 0, (int)codigo.size());
    writeExact(S, cab.c_str(), cab.size());
    writeExact(S, codigo.c_str(), codigo.size());
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <puerto>\n";
        return 1;
    }
    int port   = atoi(argv[1]);

    // Cada mensaje del log se escribe al instante (aunque se redirija a un archivo y se cierre con Ctrl+C)
    std::cout << std::unitbuf;

    // Si un cliente se cae antes de recibir su respuesta, write() no debe matar al servidor
    signal(SIGPIPE, SIG_IGN);

    int ServerSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ServerSocket == -1) {
        perror("can not create socket");
        exit(EXIT_FAILURE);
    }

    // Permite reiniciar el servidor sin esperar a que el SO libere el puerto
    int on = 1;
    setsockopt(ServerSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    struct sockaddr_in stSockAddr;
    memset(&stSockAddr, 0, sizeof(stSockAddr));
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(port);
    stSockAddr.sin_addr.s_addr = INADDR_ANY;   // escucha en todas las interfaces (necesario para 3 PCs)

    if (::bind(ServerSocket, (const struct sockaddr *)&stSockAddr, sizeof(stSockAddr)) == -1) {
        perror("error bind failed");
        close(ServerSocket);
        exit(EXIT_FAILURE);
    }
    if (listen(ServerSocket, 10) == -1) {
        perror("error listen failed");
        close(ServerSocket);
        exit(EXIT_FAILURE);
    }
    std::cout << "Servidor escuchando en el puerto " << port << " (Ctrl+C para terminar)\n";

    // ronda solo sirve para el log; no controla nada
    for (int ronda = 1; ; ronda++) {
        // Indexados por CLIENT_ID - 1
        std::vector<std::vector<Matriz>> recibidas(NUM_CLIENTES);
        int sockets[NUM_CLIENTES] = {-1, -1, -1};
        int contador = 0;
        int primero = -1;   // indice del primer cliente que llego (referencia de formas)

        // ---- Barrera: no se sigue hasta tener las 4 matrices de los 3 clientes ----
        while (contador < NUM_CLIENTES) {
            int ClientSocket = accept(ServerSocket, NULL, NULL);   // se bloquea hasta que alguien conecta
            if (ClientSocket < 0) {
                perror("error accept failed");
                continue;
            }

            try {
                std::vector<Matriz> capas;
                int id = leer_matrices(ClientSocket, capas);

                if (sockets[id - 1] != -1)              // ese cliente ya mando en esta ronda
                    throw std::runtime_error("BAD_CLIENT");
                if (primero != -1) {                    // misma forma que el primer cliente que llego
                    for (int k = 0; k < NUM_CAPAS; k++)
                        if (capas[k].rows != recibidas[primero][k].rows ||
                            capas[k].cols != recibidas[primero][k].cols)
                            throw std::runtime_error("BAD_SHAPE");
                } else {
                    primero = id - 1;
                }

                recibidas[id - 1] = capas;
                sockets[id - 1] = ClientSocket;         // queda abierto esperando su respuesta
                contador++;
                std::cout << "[Ronda " << ronda << "] cliente " << id<< " envio sus matrices (" << contador << "/" << NUM_CLIENTES << ")\n";
            } catch (const std::exception &e) {
                std::cout << "[Ronda " << ronda << "] conexion rechazada: " << e.what() << "\n";
                enviar_error(ClientSocket, e.what());
                close(ClientSocket);
            }
        }

        // ---- Los 3 llegaron: promediar y responder ----
        std::vector<Matriz> medias = calcular_medias(recibidas);
        for (int c = 0; c < NUM_CLIENTES; c++) {
            enviar_medias(sockets[c], c + 1, medias);
            close(sockets[c]);
        }
        std::cout << "[Ronda " << ronda << "] medias enviadas a los " << NUM_CLIENTES << " clientes\n";
    }

    close(ServerSocket);   // no se alcanza: el servidor corre hasta Ctrl+C
    return 0;
}
