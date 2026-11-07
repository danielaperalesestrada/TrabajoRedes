import torch.nn as nn
import torch.nn.functional as F


class Red(nn.Module):
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
        return self.fc4(x)  

    def capas(self):
        return [self.fc1, self.fc2, self.fc3, self.fc4]
