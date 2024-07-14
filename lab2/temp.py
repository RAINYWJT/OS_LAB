class DMA:
    def __init__(self, size):
        self.heap = ['#'] * size
        self.end_idx = 0
        self.allocation = []
        self.id_to_index = {}

    def malloc(self, id: int, size: int, value: list) -> bool:
        if len(self.heap) - self.end_idx < size:
            return False
        if id in self.id_to_index:
            return False
        self.heap[self.end_idx : self.end_idx + size] = value
        self.allocation.append((id, self.end_idx, size))
        self.id_to_index[id] = len(self.allocation) - 1
        self.end_idx += size
        # print(self.heap)
        return True

    def free(self, id: int) -> bool:
        if id not in self.id_to_index:
            return False
        index = self.id_to_index.pop(id)
        _, start_pos, size = self.allocation.pop(index)
        if (start_pos + size + 1 > len(self.heap)) or (self.heap[start_pos + size] == '#'):
            self.end_idx -= size
            return True
        heap_size = len(self.heap) - size
        self.heap[start_pos : start_pos + size] = []
        self.heap[heap_size : heap_size + size] = ['#'] * size
        self.end_idx -= size
        self.compact(size, start_pos)
        # print(self.heap)
        return True

    def compact(self, size, start_pos):
        new_allocation = []
        new_id_to_index = {}
        for id, pos, s in self.allocation:
            if pos >= start_pos:
                new_allocation.append((id, pos - size, s))
                new_id_to_index[id] = len(new_allocation) - 1
            else:
                new_allocation.append((id, pos , s))
                new_id_to_index[id] = len(new_allocation) - 1
        self.allocation = new_allocation
        self.id_to_index = new_id_to_index

    def data(self) -> dict:
        result = {
            id: {'start': start, 'size': size, 'value': self.heap[start:start + size]}
            for id, start, size in self.allocation
        }
        return result

    def to_list(self) -> list:
        result = []
        for id, start_pos, size in self.allocation:
            end_pos = start_pos + size - 1
            value_list = [self.heap[i] for i in range(start_pos, end_pos + 1)]
            result.append(value_list)
        return result