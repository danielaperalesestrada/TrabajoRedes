import torch.nn as nn
import torch.nn.functional as F


capa1 = 777
capa2 = 333
capa3 = 100
capa4 = 50


class Red(nn.Module):
    def __init__(self, input_dim=8):
        super().__init__()
        self.fc1 = nn.Linear(input_dim, capa1, bias=False)
        self.fc2 = nn.Linear(capa1, capa2, bias=False)
        self.fc3 = nn.Linear(capa2, capa3, bias=False)
        self.fc4 = nn.Linear(capa3, capa4, bias=False)
        self.fc5 = nn.Linear(capa4, 2, bias=False)            

    def forward(self, x):
        x = F.relu(self.fc1(x))
        x = F.relu(self.fc2(x))
        x = F.relu(self.fc3(x))
        x = F.relu(self.fc4(x))
        return self.fc5(x)  

    def capas(self):
        return [self.fc1, self.fc2, self.fc3, self.fc4, self.fc5]
