#Redes proyecto final

para correrlo:


PC client: (dentro de la carpeta client_tcp) python3 setup.py build_ext --inplace

PC servidor:   g++ -std=c++11 -O2 -Wall server.cpp -o server /// despues ./server 45000

PC 1:          python3 client/train.py 1 127.0.0.1 45000

PC 2:          python3 client/train.py 2 127.0.0.1 45000

PC 3:          python3 client/train.py 3 127.0.0.1 45000
