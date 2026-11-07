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
#include "json.hpp"

using namespace std;

#define COLOR_RESET   "\033[0m"
#define COLOR_MSG   "\033[1;33m"  
#define COLOR_MENU    "\033[2m"  
mutex mtx;


ssize_t readExact(int fd, char *buf, size_t n) {
    size_t total = 0;
    while (total < n) {
        ssize_t r = read(fd, buf + total, n - total);
        if (r <= 0) return r;   // 0 = conexión cerrada, <0 = error
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

void threadRead(int S)
{   
    int n, tamano;
    char buff[100000];
    string origin;
    string msg;
    string name_file;
    string file;

    for(;;){
        n = readExact(S, buff, 1);
        if (n <= 0) break;

        if (buff[0] == 'E'){
            n = readExact(S, buff, 11);
            if (n <= 0) break;
            buff[n] = '\0';
            tamano = atoi(buff);

            n = readExact(S, buff, tamano);
            if (n <= 0) break;
            buff[n] = '\0';
            msg = buff;


            std::cout << COLOR_MSG;
            std::cout << msg << endl;

        }

        else if (buff[0] == 'l'){
            n = readExact(S, buff, 17);
            if (n <= 0) break;
            buff[n] = '\0';
            tamano = atoi(buff);

            n = readExact(S, buff, tamano);
            if (n <= 0) break;
            buff[n] = '\0';
            msg = buff;

            nlohmann::json Json_File = nlohmann::json::parse(msg);

                        
            std::lock_guard<std::mutex> lock(mtx);

            for(auto [Clave, Valor] : Json_File.items()){

                std::cout << Clave << ": " << Valor << "\n";

            }

            //std::cout << "==================================================\n";    


            //std::cout << COLOR_MSG;
            //std::cout << msg << endl;

        }

        else if(buff[0] == 'f'){
            int tamano_user, tamano_name_file, tamano_file;
            n = readExact(S, buff, 13);
            if (n <= 0) break;
            buff[n] = '\0';
            tamano_user = atoi(buff);

            n = readExact(S, buff, tamano_user); //lectura del usuario
            if (n <= 0) break;
            buff[n] = '\0';
            origin = buff;


            n = readExact(S, buff, 13);
            if (n <= 0) break;
            buff[n] = '\0';
            tamano_name_file = atoi(buff);

            n = readExact(S, buff, tamano_name_file); //lectura del name file
            if (n <= 0) break;
            buff[n] = '\0';
            name_file = buff;

            n = readExact(S, buff, 25);
            if (n <= 0) break;
            buff[n] = '\0';
            tamano_file = atoi(buff);

            n = readExact(S, buff, tamano_file); //lectura del file
            if (n <= 0) break;

            int Pos_Punto = name_file.rfind('.');
            std::string Nombre_Base = name_file.substr(0, Pos_Punto);
            std::string Extension  = name_file.substr(Pos_Punto);
            string new_name = Nombre_Base + "_" + origin + Extension;

            std::ofstream Out(new_name, std::ios::binary);

            Out.write(buff,tamano_file);
            Out.close();

            std::cout << COLOR_MSG;
            std::cout<<"\nArchivo recibido de "+origin + " guardado como: " + new_name + "\n";
        }

        else {
        
            n = readExact(S, buff, 7);
            if (n <= 0) break;
            buff[n] = '\0';
            tamano = atoi(buff);

            n = readExact(S, buff, tamano);
            if (n <= 0) break;
            buff[n] = '\0';
            origin = buff;

            n = readExact(S, buff, 11);
            if (n <= 0) break;
            buff[n] = '\0';
            tamano = atoi(buff);

            n = readExact(S, buff, tamano);
            if (n <= 0) break;
            buff[n] = '\0';
            msg = buff;

            std::cout << COLOR_MSG;
            std::cout << origin << ": " << msg << endl;
            std::cout << "> " << COLOR_RESET;
        }
    }
  
}

void Write_M(int S, string nombre,string mssg){

    string mssgPacket;

    int size_name = nombre.length();
    int size_mssg = mssg.length();

    if(nombre == "B"){
       mssgPacket = "B" + zeroPad(size_mssg, 11) + mssg;
    }
    else{
        mssgPacket = "M" + zeroPad(size_name,7)+ nombre +zeroPad(size_mssg, 11) + mssg;
    }

    writeExact(S, mssgPacket.c_str(), mssgPacket.size());
}


void Write_File(int S, string nombre,string file_name){

    std::cout<<"Nombre del file: " ;
    std::cout << "> " << COLOR_RESET;;
    getline(std::cin,file_name);

    std::ifstream File(file_name, std::ios::binary);

    std::string File_Contenido( (std::istreambuf_iterator<char>(File)), std::istreambuf_iterator<char>());
    
    if(File_Contenido.length() > 99999) File_Contenido.resize(99999);

   
    string filePacket;

    int size_name = nombre.length();
    int size_name_file = file_name.length();
    int size_file = File_Contenido.length();


    filePacket = "F" + zeroPad(size_name, 13)+ nombre + zeroPad(size_name_file, 13) + file_name + zeroPad(size_file, 25) + File_Contenido;
    
    writeExact(S, filePacket.c_str(), filePacket.size());
}


void Write(int S, char op, string nombre){
   //string nickname;
    char buff[1000];
    int n, tamano;

    int size_name = nombre.length();

    string namePacket = "N" + zeroPad(size_name, 7) + nombre;

    writeExact(S, namePacket.c_str(), namePacket.size());
}

int main(int argc, char *argv[])
{   
    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in stSockAddr;

    // Crear socket
    int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (SocketFD == -1)
    {
        perror("Cannot create socket");
        exit(EXIT_FAILURE);
    }


    memset(&stSockAddr, 0, sizeof(stSockAddr));

    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(port);


    // IP del servidor
    int Res = inet_pton(AF_INET, ip, &stSockAddr.sin_addr);

    if (Res <= 0)
    {
        perror("inet_pton");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }


    // CONECTAR
    Res = connect(SocketFD, (const struct sockaddr *)&stSockAddr,sizeof(struct sockaddr_in));

    if (Res == -1)
    {
        perror("connect failed");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    std::cout << "Connected to server!" << endl;

    string name;


    thread(threadRead, SocketFD).detach();


    int opcion;
    string mssg;
    while (true)
    {   std::cout << COLOR_MENU;
        std::cout<< "Elija una opcion: "<<endl;
        std::cout<<"1. Registrarse, 2. Enviar mensaje, 3. Quit, 4.Enviar file, 5. Lista de usuarios " <<endl;
        std::cout << "> "<< COLOR_RESET; 
        std::cin>> opcion;

        switch (opcion)
        {
        case 1:{
            std::cout << COLOR_MENU;
            std::cout<<"Ingrese nombre: " ;
            std::cout << "> "<< COLOR_RESET;
            std::cin.ignore();
            getline(std::cin,name);

            Write(SocketFD,'N', name);
            break;}

        case 2: {
            string usuario;
            std::cout << COLOR_MENU;
            std::cout<<"A quien va tu mensaje?"<<endl;
            std::cout << "> ";
            std::cin>>usuario; std::cin.ignore();
            std::cout<<"Ingrese mensaje: " ;
            std::cout << "> " << COLOR_RESET;;
            getline(std::cin,mssg);
            Write_M(SocketFD, usuario, mssg);
            break;
        }

        case 3: {
            writeExact(SocketFD, "Q", 1);
            shutdown(SocketFD, SHUT_WR);
            close(SocketFD);
            return 0;}
        case 4:{
            string usuario;
            std::cout << COLOR_MENU;
            std::cout<<"A quien quieres enviar?"<<endl;
            std::cout << "> ";
            std::cin>>usuario; std::cin.ignore();
            

            Write_File(SocketFD, usuario, mssg);
            break;
        }
        
        case 5: {
            writeExact(SocketFD, "L", 1);
            break;}
        }
        

    }

    return 0;
}