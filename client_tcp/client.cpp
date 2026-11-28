#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <map>
#include <cmath>
#include <stdexcept>

namespace py = pybind11;

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


std::string zeroPad(int number, int size)
{
    std::string str = std::to_string(number);

    if (str.length() >= size)
        return str;

    return std::string(size - str.length(), '0') + str;
}



std::string armarCabecera(char type, int rows, int cols, int payload_len){
    return std::string(1, type) + zeroPad(rows, 4)+ zeroPad(cols, 4)+ zeroPad(payload_len, 8);
}

bool Write(int S, const std::vector<py::array_t<double>> &matrices){
    for (const auto &m : matrices){

        py::array_t<double, py::array::c_style | py::array::forcecast> M(m); // necesario para pasar de numpy python a numpy c++

        int rows = (int)M.shape(0);
        int cols = (int)M.shape(1);
        int bytes = rows * cols * (int)sizeof(double);

        std::string cab = armarCabecera('M', rows, cols, bytes);
        if (writeExact(S, cab.c_str(), cab.size()) <= 0) return false;

        if (writeExact(S, (const char *)M.data(), bytes) <= 0) return false;
    }
    return true;
}

int leerCampo(int S, int size){
    char buff[16];
    if (readExact(S, buff, size) <= 0)
        throw std::runtime_error("Conexion cerrada por el servidor");
    buff[size] = '\0';
    return atoi(buff);
}

std::vector<py::array_t<double>> leer_medias(int S){
    std::vector<py::array_t<double>> medias;

    for (int capa = 0; capa < 5 ; capa++){
        char type;
        if (readExact(S, &type, 1) <= 0)
            throw std::runtime_error("Conexion cerrada por el servidor");

        
        int rows = leerCampo(S, 4);
        int cols  = leerCampo(S, 4);
        int payload_len = leerCampo(S, 8);

        if (type == 'E'){
            std::string msg(payload_len, '\0');
            readExact(S, &msg[0], payload_len);
            throw std::runtime_error("Servidor: " + msg);
        }

        if (type != 'A')
            throw std::runtime_error("Tipo de mensaje desconocido");

        if (payload_len != rows * cols * (int)sizeof(double))
            throw std::runtime_error("PAYLOAD_LEN no coincide con ROWS x COLS x 8");

        py::array_t<double> M({rows, cols});
        if (readExact(S, (char *)M.mutable_data(), payload_len) <= 0)
            throw std::runtime_error("Conexion cerrada leyendo la matriz");

        medias.push_back(M);
    }

    return medias;
}


std::vector<py::array_t<double>> trasladar_matrices(std::string ip, int port, std::vector<py::array_t<double>> matrices)
{
    
    struct sockaddr_in stSockAddr;

    int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (SocketFD == -1)
        throw std::runtime_error("No se pudo crear el socket");

    memset(&stSockAddr, 0, sizeof(stSockAddr));
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &stSockAddr.sin_addr) <= 0){
        close(SocketFD);
        throw std::runtime_error("IP invalida: " + ip);
    }

    if (connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in)) == -1){
        close(SocketFD);
        throw std::runtime_error("No se pudo conectar a " + ip + ":" + std::to_string(port));
    }

    try{
        if (!Write(SocketFD, matrices))
            throw std::runtime_error("Fallo al enviar las matrices");

        std::vector<py::array_t<double>> medias = leer_medias(SocketFD);
        close(SocketFD);
        return medias;
    } catch (...){
        close(SocketFD);
        throw; 
    }
}

PYBIND11_MODULE(tcp, m) {
    m.def("trasladar_matrices", &trasladar_matrices,
          "Envia 4 matrices al servidor y devuelve las 4 medias");
}
