class DMA:
    def __init__(self, size):
        self.heap = ['#'] * size
        self.end_idx = 0
        self.allocation = {}

    def malloc(self, id: int, size: int, value: list) -> bool:
        if len(self.heap) - self.end_idx < size:
            return False
        if id in self.allocation:
            return False
        self.heap[self.end_idx : self.end_idx + size] = value
        self.allocation[id] = (self.end_idx, size)
        self.end_idx += size
        return True

    def free(self, id: int) -> bool:
        if id not in self.allocation:
            return False
        start_pos, size = self.allocation.pop(id)
        if (start_pos + size + 1 > len(self.heap)) or (self.heap[start_pos + size] == '#'):
            self.end_idx -= size            
            return True
        heap_size = len(self.heap) - size
        self.heap[start_pos : start_pos + size] = []
        self.heap[heap_size : heap_size + size] = ['#'] * size
        self.end_idx -= size
        self.compact(size, start_pos)
        return True

    def compact(self, size, start_pos):
        self.allocation = {
            k: (v[0]-size, v[1]) if v[0] > start_pos else v 
            for k, v in  self.allocation.items()
        }

    def data(self) -> dict:
        result = {
            id: {'start': start, 'size': size, 'value': self.heap[start:start + size]}
            for id, (start, size) in self.allocation.items()
        }
        return result
