class DMA:
    def __init__(self, size):
        self.heap = ['#'] * size
        self.allocation = {}

    def malloc(self, id: int, size: int, value: list) -> bool:
        # id has been used!
        if id in self.allocation:
            return False
        
        # avoid internal fragment
        heap_sections = [self.heap[start:start+length] for start, length in self.allocation.values()]
        heap_count = sum([section.count('#') for section in heap_sections])

        # print(self.heap.count('#'))
        if self.heap.count('#') - heap_count < size:
            return False
        # try to do the allocation for first time
        for i in range(len(self.heap) - size + 1):
            if all(self.heap[j] == '#' for j in range(i, i + size)):
                self.heap[i: i + size] = value
                self.allocation[id] = (i, size)
                return True
        
        #if there is not enough space 
        self.compact()

        #try to allocate for the second time
        for i in range(len(self.heap) - size + 1):
            if all(self.heap[j] == '#' for j in range(i, i + size)):
                self.heap[i: i + size] = value
                self.allocation[id] = (i, size)
                return True
        return False

    def free(self, id: int) -> bool:
        #not found the free id:
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

    def to_list(self) -> list:
        result = []
        for id, (start_pos, size) in self.allocation.items():
            end_pos = start_pos + size - 1
            value_list = [str(self.heap[i]) for i in range(start_pos, end_pos + 1)]
            result.append(value_list)
        return result 

    def data(self) -> dict:
        result = {}
        for id, (start, size) in self.allocation.items():
            value = self.heap[start:start + size]
            result[id] = {'start': start, 'size': size, 'value': value}
        output = "{\n"
        for key, value in result.items():
            output += f"   {key}: {value},\n"
        output += "}"
        return output
    
