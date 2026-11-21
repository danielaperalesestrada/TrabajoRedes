/* Server code in C */
 
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>
 #include "json.hpp"

  #include <iostream>
  #include <map>
  #include <string>
  #include <thread> 
  #include <mutex>

  using namespace std; 
  

  map<string,int> ListOfCli;
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


void ThreadReadClient(int S){
    string nickname;
    char buff[100000];
    int n, tamano;
    readExact(S,buff,1);
    while (buff[0]!='N'){
      readExact(S,buff,1);
    }
    n = readExact(S,buff,7); 
    buff[n] = '\0';
    tamano = atoi(buff);
    n = readExact(S,buff,tamano); 
    buff[n] = '\0';
    nickname = buff;
    {
      lock_guard<mutex> lock(mtx);
      cout << buff << endl;
      ListOfCli[nickname] = S;
    }

    string destination;
    
    string msg;
    string filename;
    string file;
    for (;;){
      n = readExact(S,buff,1); 

      if (buff[0] == 'M'){
        n = readExact(S,buff,7); 
        buff[n] = '\0';
        tamano = atoi(buff);
        n = readExact(S,buff,tamano); 
        buff[n] = '\0';
        destination = buff;

        n = readExact(S,buff,11); 
        buff[n] = '\0';
        tamano = atoi(buff);
        n = readExact(S,buff,tamano); 
        buff[n] = '\0';
        msg = buff;

        string dataStructure = "";

        dataStructure = dataStructure + 'm';
        dataStructure = dataStructure + zeroPad(nickname.size(),7);
        dataStructure = dataStructure + nickname ;
        dataStructure = dataStructure + zeroPad(msg.size(),11);
        dataStructure = dataStructure + msg;

        {
          lock_guard<mutex> lock(mtx);
          map<string,int>::iterator it = ListOfCli.find(destination);
          if (it != ListOfCli.end()) {
            writeExact(it->second, dataStructure.c_str(), dataStructure.size());
          }
          else {

            dataStructure = "";
            string errorMessage = destination + " no esta conectado.";
            dataStructure = 'E' + zeroPad(errorMessage.size(),11) + errorMessage;
            map<string,int>::iterator it2 = ListOfCli.find(nickname);
            writeExact(it2->second, dataStructure.c_str(), dataStructure.size());
          }
          cout<<dataStructure<<endl;
        }


      }
      else if (buff[0] == 'B'){

        n = readExact(S,buff,11); 
        buff[n] = '\0';
        tamano = atoi(buff);
        n = readExact(S,buff,tamano); 
        buff[n] = '\0';
        msg = buff;
        string dataStructure="";

        dataStructure = dataStructure + 'b';
        dataStructure = dataStructure + zeroPad(nickname.size(),7);
        dataStructure = dataStructure + nickname ;
        dataStructure = dataStructure + zeroPad(msg.size(),11);
        dataStructure = dataStructure + msg;

        {
          lock_guard<mutex> lock(mtx);
          for (map<string,int>::iterator it = ListOfCli.begin(); it != ListOfCli.end(); ++it){
            writeExact(it->second, dataStructure.c_str(), dataStructure.size());
          }
          cout<<dataStructure<<endl;
        }
      }
      else if (buff[0] == 'Q'){
        {
          lock_guard<mutex> lock(mtx);
          ListOfCli.erase(nickname);
        }
        close(S);
        break;
      }      
      /*
      else if (buff[0] == 'L'){
        {
          lock_guard<mutex> lock(mtx);
          string dataStructure="";

          map<string,int>::iterator it1 = ListOfCli.find(nickname);

          for (map<string,int>::iterator it = ListOfCli.begin(); it != ListOfCli.end(); ++it){
            dataStructure = dataStructure + "," + it->first;
          }
          dataStructure = "l" + zeroPad(dataStructure.size(),17) + dataStructure;

          writeExact(it1->second, dataStructure.c_str(), dataStructure.size());
          cout<<dataStructure<<endl;
        }
      } */
       else if (buff[0] == 'L'){

        lock_guard<mutex> lock(mtx);
          nlohmann::json Json_File;

          int Contador = 1;

          for(auto Par : ListOfCli){

              Json_File["Cliente_" + std::to_string(Contador)] = Par.first; 
              Contador++;

          }

          std::string Json_Texto = Json_File.dump();

          std::string Packet = "l" + zeroPad(Json_Texto.length(), 17) + Json_Texto;
          cout<<Packet<<endl;
          writeExact(S, Packet.c_str(), Packet.length());
      }

      else if (buff[0] == 'F'){
        n = readExact(S,buff,13); 
        buff[n] = '\0';
        tamano = atoi(buff);
        n = readExact(S,buff,tamano); 
        buff[n] = '\0';
        destination = buff;

        n = readExact(S,buff,13); 
        buff[n] = '\0';
        tamano = atoi(buff);
        n = readExact(S,buff,tamano); 
        buff[n] = '\0';
        filename = buff;

        n = readExact(S,buff,25); 
        buff[n] = '\0';
        tamano = atoi(buff);
        n = readExact(S,buff,tamano); 

        //file = buff;

        string dataStructure = "";

        dataStructure = dataStructure + 'f';
        dataStructure = dataStructure + zeroPad(nickname.size(),13);
        dataStructure = dataStructure + nickname ;
        dataStructure = dataStructure + zeroPad(filename.size(),13);
        dataStructure = dataStructure + filename;
        dataStructure = dataStructure + zeroPad(tamano,25);
        dataStructure.append(buff, tamano);

        {
          lock_guard<mutex> lock(mtx);
          map<string,int>::iterator it = ListOfCli.find(destination);
          if (it != ListOfCli.end()) {
            writeExact(it->second, dataStructure.c_str(), dataStructure.size());
          }
          else {

            dataStructure = "";
            string errorMessage = destination + " no esta conectado.";
            dataStructure = 'E' + zeroPad(errorMessage.size(),11) + errorMessage;
            map<string,int>::iterator it2 = ListOfCli.find(nickname);
            writeExact(it2->second, dataStructure.c_str(), dataStructure.size());
          }
          cout<<dataStructure<<endl;
        }


      }
    }

  }

  int main(int argc, char *argv[])
  {
    int port = atoi(argv[1]);
    struct sockaddr_in stSockAddr;
    int ServerSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    char buffer[256];
    int n;
 
    if(-1 == ServerSocket)
    {
      perror("can not create socket");
      exit(EXIT_FAILURE);
    }
 
    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
 
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(port);
    stSockAddr.sin_addr.s_addr = INADDR_ANY;
 
    if (::bind(ServerSocket,(const struct sockaddr *)&stSockAddr,sizeof(stSockAddr)) == -1)
    {
      perror("error bind failed");
      close(ServerSocket);
      exit(EXIT_FAILURE);
    }
 
    if(-1 == listen(ServerSocket, 10))
    {
      perror("error listen failed");
      close(ServerSocket);
      exit(EXIT_FAILURE);
    }
 
    for(;;)
    {
      int ClientSocket = accept(ServerSocket, NULL, NULL);
      if(0 > ClientSocket)
      {
        perror("error accept failed");
        close(ServerSocket);
        exit(EXIT_FAILURE);
      }
      thread (ThreadReadClient,ClientSocket).detach();

    }
 
    close(ServerSocket);
    return 0;
  }