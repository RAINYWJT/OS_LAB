class DMA:
    def __init__(self, size):
        self.heap = ['#'] * size
        self.allocation = {}
        self.vis_allocation = {}
        self.vis_space = 0

    def malloc(self, id: int, size: int, value: list) -> bool:
        if id in self.allocation:
            return False
        
        # avoid internal fragment
        heap_sections = [self.heap[start:start+length] for start, length in self.allocation.values()]
        heap_count = sum([section.count('#') for section in heap_sections])
        if self.heap.count('#') - self.vis_space - heap_count - size < 0:
            return False
        
        for i in range(len(self.heap) - size + 1):
            if all(self.heap[j] == '#' for j in range(i, i + size)):
                self.heap[i: i + size] = value
                self.allocation[id] = (i, size)
                return True
        
        self.vis_allocation[id] = (id, size, value)
        self.vis_space += size
        return True

    def free(self, id: int) -> bool:
        if id in self.vis_allocation:
            temp_id, temp_size, temp_value = self.vis_allocation[id]
            self.vis_space -= temp_size
            del self.vis_allocation[id]
            return True

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
    
    def end(self):
        if self.vis_allocation == None:
            return True
        self.compact()
        for id, (id_, size, value) in self.vis_allocation.items():
            for i in range(len(self.heap) - size + 1):
                if all(self.heap[j] == '#' for j in range(i, i + size)):
                    self.heap[i: i + size] = value
                    self.allocation[id] = (i, size)
                    break
        return True

'''
dma_test = DMA(10)

workload = [
    {'type': 'malloc', 'id': 1, 'size': 4, 'value': ['a', 'b', 'c', 'd']},
    {'type': 'malloc', 'id': 2, 'size': 4, 'value': ['1', '1', '1', '1']},
    {'type': 'free', 'id': 1},
    {'type': 'malloc', 'id': 3, 'size': 5, 'value': ['2', '2', '2', '2', '2']},
    {'type': 'free', 'id': 3},
    {'type': 'malloc', 'id': 4, 'size': 3, 'value': ['3', '3', '3']},
    {'type': 'malloc', 'id': 5, 'size': 3, 'value': ['4', '4', '4']},
    {'type': 'free', 'id': 2},
    {'type': 'free', 'id': 5},
    {'type': 'malloc', 'id': 6, 'size': 7, 'value': ['2', '2', '2', '2', '2', '2', '2']},
    {'type': 'free', 'id': 4},
    {'type': 'free', 'id': 6},
    {'type': 'end'}
]


workload1 = [
    {'type': 'malloc', 'id': 1, 'size': 2, 'value': ['a', 'b']},
    {'type': 'malloc', 'id': 2, 'size': 2, 'value': ['a', 'b']},
    {'type': 'malloc', 'id': 3, 'size': 2, 'value': ['a', 'b']},
    {'type': 'malloc', 'id': 4, 'size': 2, 'value': ['a', 'b']},
    {'type': 'malloc', 'id': 5, 'size': 2, 'value': ['a', 'b']},
    {'type': 'free', 'id': 1},
    {'type': 'free', 'id': 3},
    {'type': 'free', 'id': 5},
    {'type': 'malloc', 'id': 6, 'size': 3, 'value': ['1', '1', '1']},
    {'type': 'malloc', 'id': 7, 'size': 3, 'value': ['2', '2', '2']},
    {'type': 'end'}
]


import time

start_time = time.time()
for i in range(1):
    for request in workload1:
        if request['type'] == 'malloc':
            dma_test.malloc(request['id'], request['size'], request['value'])
        elif request['type'] == 'free':
            dma_test.free(request['id'])
        elif request['type'] == 'end':
            break  
dma_test.end() 

end_time = time.time()  
run_time = end_time - start_time  
print("running time:", run_time, "s")
print("Compact function called:", dma_test.compact_count)
print(dma_test.data())
'''