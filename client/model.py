import torch.nn as nn
import torch.nn.functional as F


class Red(nn.Module):
    # 8 -> 100 -> 150 -> 50 -> 2, ReLU en las ocultas.
    # bias=False: las 4 matrices de pesos son TODA la red (lo unico que se promedia).
    def __init__(self, input_dim=8):
        super().__init__()
        self.fc1 = nn.Linear(input_dim, 100, bias=False)  
        self.fc2 = nn.Linear(100, 150, bias=False)        
        self.fc3 = nn.Linear(150, 50, bias=False)       
        self.fc4 = nn.Linear(50, 2, bias=False)         

    def forward(self, x):
        x = F.relu(self.fc1(x))
        x = F.relu(self.fc2(x))
        x = F.relu(self.fc3(x))
        return self.fc4(x)  # logits, sin ReLU (CrossEntropyLoss aplica softmax)

    def capas(self):
        return [self.fc1, self.fc2, self.fc3, self.fc4]
