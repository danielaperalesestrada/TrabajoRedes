import os
import sys
import numpy as np
import torch
import torch.nn as nn
from torch.utils.data import DataLoader, TensorDataset
from model import Red
#import client_tcp


CLIENT_ID = 1 
IP = "127.0.0.1"
PORT = 45000

EPOCHS = 20
BATCH_SIZE = 5
LR = 0.001
TEST_PER = 0.15

# ---------------- Data ----------------
ruta = "../data/data.csv" #checar si aqui se presentan errores
data = np.loadtxt(ruta, delimiter=",", skiprows=1, dtype=np.float32)

np.random.default_rng(0).shuffle(data)  #aqui reordenamos las filas de la data, es para evitar sesgos de aprendizaje
X = data[:, :8] 
y = data[:, 8].astype(np.int64)


n_test = int(len(X) * TEST_PER) 
X_train, y_train = X[:-n_test], y[:-n_test] 
X_test, y_test = X[-n_test:], y[-n_test:]

# Normalizacion del dataset
media, std = X_train.mean(axis=0), X_train.std(axis=0)
X_train = (X_train-media) /std
X_test = (X_test-media) /std

# Cada cliente toma su tercio
partes = np.array_split(np.arange(len(X_train)), 3)[CLIENT_ID - 1]
X_mio = torch.from_numpy(X_train[partes]) #convirtiendolo a tensores , Comparado con torch.tensor(), esta función comparte la misma memoria RAM que el arreglo de NumPy original
y_mio = torch.from_numpy(y_train[partes])
X_test, y_test = torch.from_numpy(X_test), torch.from_numpy(y_test)

loader = DataLoader(TensorDataset(X_mio, y_mio), batch_size=BATCH_SIZE, shuffle=True)

# ---------------- Modelo ----------------
torch.manual_seed(42)  # mismos pesos iniciales en los 3 clientes
model = Red(8) 
capas = model.capas()
criterion = nn.CrossEntropyLoss()
optimizer = torch.optim.Adam(model.parameters(), lr=LR)


def evaluar(Xe, ye):
    model.eval()
    with torch.no_grad():
        logits = model(Xe)
        loss = criterion(logits, ye).item()
        acc = (logits.argmax(dim=1) == ye).float().mean().item()
    return loss, acc


# ---------------- Entrenamiento ----------------
for epoch in range(1, EPOCHS + 1):
    model.train()
    for batch_x, batch_y in loader:  # cada xb es un batch de 5 x 8
        optimizer.zero_grad()
        loss = criterion(model(batch_x), batch_y)
        loss.backward()
        optimizer.step()

    # [C++] Intercambio con el servidor (una vez por epoca):
    # 1) sacar las 4 matrices como float64 para C++
    # mats = [c.weight.detach().numpy().astype(np.float64) for c in capas]
    # 2) abre socket, envia, espera las 4 medias, cierra socket
    # medias = comm.trasladar_matrices(IP, PORT, CLIENT_ID, mats)
    # 3) reemplazar los pesos de la red por las medias
    # with torch.no_grad():
    #     for c, M in zip(capas, medias):
    #         c.weight.copy_(torch.from_numpy(M))

    # Se evalua despues del intercambio -> metricas del modelo promediado
    loss_tr, acc_tr = evaluar(X_mio, y_mio)
    print(f"[Cliente {CLIENT_ID}] Epoca {epoch:2d}/{EPOCHS}  loss={loss_tr:.4f}  acc={acc_tr:.4f}")

# ---------------- Prueba final ----------------
loss_te, acc_te = evaluar(X_test, y_test)
print(f"\n[Cliente {CLIENT_ID}] TEST  loss={loss_te:.4f}  acc={acc_te:.4f}  (n={len(y_test)})")
