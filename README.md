#Redes proyecto final

para correrlo:


PC client: (dentro de la carpeta client_tcp) python3 setup.py build_ext --inplace

PC servidor:   cd server && make && ./server 45000

PC 1:          python3 client/train.py 1 IP

PC 2:          python3 client/train.py 2 IP

PC 3:          python3 client/train.py 3 IP
