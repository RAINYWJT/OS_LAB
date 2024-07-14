import numpy as np
from sklearn.linear_model import LinearRegression
from sklearn.svm import LinearSVR
from sklearn.ensemble import RandomForestRegressor

class SmartDMA:
    def __init__(self, size):
        self.heap = ['#'] * size
        self.allocation = {}
        self.usage_history = []  
        
    def malloc(self, id: int, size: int, value: list) -> bool:
        if id in self.allocation:
            return False

        predicted_size = self.predict_memory_size()
        # print(predicted_size,end=',')
        if self.heap.count('#') < predicted_size:
            return False

        for i in range(len(self.heap) - size + 1):
            if all(self.heap[j] == '#' for j in range(i, i + size)):
                self.heap[i: i + size] = value
                self.allocation[id] = (i, size)
                self.usage_history.append(size)
                return True

        self.compact()

        for i in range(len(self.heap) - size + 1):
            if all(self.heap[j] == '#' for j in range(i, i + size)):
                self.heap[i: i + size] = value
                self.allocation[id] = (i, size)
                self.usage_history.append(size)
                return True

        return False

    def free(self, id: int) -> bool:
        if id not in self.allocation:
            return False

        start_pos, size = self.allocation[id]
        self.heap[start_pos: start_pos + size] = ['#'] * size
        del self.allocation[id]
        return True

    def compact(self) -> None:
        new_heap = []
        new_allocation = {}
        for id, (start_pos, size) in sorted(self.allocation.items(), key=lambda item: item[1][0]):
            new_allocation[id] = (len(new_heap), size)
            new_heap.extend(self.heap[start_pos: start_pos + size])
        free_space = ['#'] * (len(self.heap) - len(new_heap))
        new_heap.extend(free_space)
        self.heap = new_heap
        self.allocation = new_allocation

    def predict_memory_size(self) -> int:
        if not self.usage_history:
            return 2  

        X = np.array(range(len(self.usage_history))).reshape(-1, 1)
        y = np.array(self.usage_history)

        model = RandomForestRegressor(n_estimators=10,max_depth=10)
        model.fit(X, y)
    
        predicted_size = int(model.predict([[len(self.usage_history)]]))
    
        return max(predicted_size, 1)


    def to_list(self) -> list:
        result = []
        for id, (start_pos, size) in self.allocation.items():
            end_pos = start_pos + size - 1
            value_list = [str(self.heap[i]) for i in range(start_pos, end_pos + 1)]
            result.append(value_list)
        return result
