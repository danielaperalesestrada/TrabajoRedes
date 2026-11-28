

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
#define NUM_CAPAS 5

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

std::string armarCabecera(char type, int rows, int cols, int payload_len) {
    return std::string(1, type) + zeroPad(rows, 4) + zeroPad(cols, 4) + zeroPad(payload_len, 8);
}

int leerCampo(int S, int size) {
    char buff[16];
    if (readExact(S, buff, size) <= 0)
        throw std::runtime_error("Conexion cerrada por el cliente");
    buff[size] = '\0';
    return atoi(buff);
}


void leer_matrices(int S, std::vector<Matriz> &capas) {
    int id = 0;
    for (int capa = 0; capa < NUM_CAPAS; capa++) {
        char type;
        if (readExact(S, &type, 1) <= 0)
            throw std::runtime_error("Conexion cerrada por el cliente");


        
            int rows  = leerCampo(S, 4);
        int cols = leerCampo(S, 4);
        int payload_len = leerCampo(S, 8);

        if (type != 'M' || payload_len != rows * cols * (int)sizeof(double))
            throw std::runtime_error("BAD_HEADER");


        Matriz M;
        M.rows = rows;
        M.cols = cols;
        M.data.resize(rows * cols);
        if (readExact(S, (char *)M.data.data(), payload_len) <= 0)
            throw std::runtime_error("Conexion cerrada leyendo la matriz");

        capas.push_back(M);
    }

}

std::vector<Matriz> calcular_medias(const std::vector<std::vector<Matriz>> &recibidas) {
    std::vector<Matriz> medias = recibidas[0]; 
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

void enviar_medias(int S, const std::vector<Matriz> &medias) {
    for (const Matriz &M : medias) {
        int bytes = M.rows * M.cols * (int)sizeof(double);
        std::string cab = armarCabecera('A', M.rows, M.cols, bytes);
        writeExact(S, cab.c_str(), cab.size());
        writeExact(S, (const char *)M.data.data(), bytes);
    }
}

void enviar_error(int S, const std::string &codigo) {
    std::string cab = armarCabecera('E', 0, 0, (int)codigo.size());
    writeExact(S, cab.c_str(), cab.size());
    writeExact(S, codigo.c_str(), codigo.size());
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <puerto>\n";
        return 1;
    }
    int port   = atoi(argv[1]);

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
    stSockAddr.sin_addr.s_addr = INADDR_ANY;   

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

    for (int ronda = 1; ; ronda++) {

        std::vector<std::vector<Matriz>> recibidas(NUM_CLIENTES);
        int sockets[NUM_CLIENTES] = {-1, -1, -1};
        int contador = 0;
           

        while (contador < NUM_CLIENTES) {
            int ClientSocket = accept(ServerSocket, NULL, NULL);  
            if (ClientSocket < 0) {
                perror("error accept failed");
                continue;
            }

            try {
                std::vector<Matriz> capas;
                leer_matrices(ClientSocket, capas);

                if (contador > 0) {
                    for (int k = 0; k < NUM_CAPAS; k++) {
                        if (capas[k].rows != recibidas[0][k].rows ||
                            capas[k].cols != recibidas[0][k].cols) {
                            throw std::runtime_error("BAD_SHAPE");
                        }
                    }
                }


                recibidas[contador ] = capas;
                sockets[contador] = ClientSocket;      
                contador++;
                std::cout << "[Ronda " << ronda << "]"<< " envio sus matrices (" << contador << "/" << NUM_CLIENTES << ")\n";
            } catch (const std::exception &e) {
                std::cout << "[Ronda " << ronda << "] conexion rechazada: " << e.what() << "\n";
                enviar_error(ClientSocket, e.what());
                close(ClientSocket);
            }
        }

        std::vector<Matriz> medias = calcular_medias(recibidas);
        for (int c = 0; c < NUM_CLIENTES; c++) {
            enviar_medias(sockets[c], medias);
            close(sockets[c]);
        }
        std::cout << "[Ronda " << ronda << "] medias enviadas a los " << NUM_CLIENTES << " clientes\n";
    }

    close(ServerSocket); 
    return 0;
}
