# Redes Trabajo Final

El proyecto se ejecuta con un servidor y tres clientes. Cada cliente entrena con una parte de los datos y envía sus pesos al servidor. Cuando llegan los tres, el servidor promedia los pesos y los devuelve.

## Preparar el servidor

En Linux, abre una terminal en la carpeta `TrabajoRedes` y compila el servidor. Los comandos de instalación usan `apt`:

```bash
sudo apt update
sudo apt install build-essential
g++ -std=c++11 -O2 -Wall server/server.cpp -o server/server
```

## Preparar el cliente

Desde `TrabajoRedes`, instala las dependencias. Si vas a correr los tres clientes en la misma PC, basta con hacerlo una vez.

```bash
sudo apt update
sudo apt install build-essential python3 python3-dev python3-venv
python3 -m venv .venv-redes
source .venv-redes/bin/activate
python -m pip install --upgrade pip setuptools
python -m pip install numpy pybind11
python -m pip install torch --index-url https://download.pytorch.org/whl/cpu
```

En esa misma terminal, compila el módulo TCP del cliente:

```bash
cd client_tcp
python setup.py build_ext --inplace --force
cd ..
```

## Ejecucion del proyecto

Abre cuatro terminales en `TrabajoRedes`. Arranca primero el servidor y luego ejecuta cada cliente en su propia terminal.

| Terminal | Comando |
| --- | --- |
| Servidor | `./server/server 45000` |
| Cliente 1 | `.venv-redes/bin/python client/train.py 1 127.0.0.1 45000` |
| Cliente 2 | `.venv-redes/bin/python client/train.py 2 127.0.0.1 45000` |
| Cliente 3 | `.venv-redes/bin/python client/train.py 3 127.0.0.1 45000` |

Si el servidor muestra `1/3` o `2/3`, todavía está esperando al resto. Cuando complete la ronda, verás `medias enviadas a los 3 clientes`. Cada cliente mostrará sus resultados en la línea `TEST`. Para cerrar el servidor, presiona Ctrl+C.

### Uso multiples computadoras

Prepara el cliente en cada PC y asegúrate de que todas tengan el mismo archivo `data/data.csv`. En los comandos de los clientes, cambia `127.0.0.1` por la IP del servidor. Puedes consultarla con `hostname -I` en la máquina que ejecuta el servidor. Usa un ID distinto en cada cliente (1, 2 y 3) y el mismo puerto en todos.
