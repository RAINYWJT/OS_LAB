class Node:
    def __init__(self, id, start, size, prev=None, next=None):
        self.id = id
        self.start = start
        self.size = size
        self.end = start + size
        self.prev = prev
        self.next = next

class DMA:
    def __init__(self, size):
        self.heap = ['#'] * size
        
        self.end_idx = 0
        self.allocation = {}
        self.head = None
        self.tail = None

    def malloc(self, id: int, size: int, value: list) -> bool:
        if len(self.heap) - self.end_idx < size:
            return False
        if id in self.allocation:
            return False
        self.heap[self.end_idx : self.end_idx + size] = value
        node = Node(id, self.end_idx, size)
        if self.head is None:
            self.head = self.tail = node
        else:
            self.tail.next = node
            node.prev = self.tail
            self.tail = node
        self.allocation[id] = node
        self.end_idx += size
        # print(self.heap)
        # print(self.allocation)
        return True

    def free(self, id: int) -> bool:
        if id not in self.allocation:
            return False
        node = self.allocation.pop(id)
        if node.prev is not None:
            node.prev.next = node.next
        if node.next is not None:
            node.next.prev = node.prev
        if node == self.head:
            self.head = node.next
        if node == self.tail:
            self.tail = node.prev
        self.heap[node.start : node.end] = ['#'] * node.size
        cur = node.next
        while cur is not None:
            self.heap[cur.start - node.size : cur.end - node.size] = self.heap[cur.start : cur.end]
            cur.start -= node.size
            cur.end -= node.size
            cur = cur.next
        self.heap[self.end_idx - node.size : self.end_idx] = ['#'] * node.size
        self.end_idx -= node.size
        # print(self.heap)
        # print(self.allocation)
        return True

    def data(self) -> dict:
        result = {
            id: {'start': node.start, 'size': node.size, 'value': self.heap[node.start:node.start + node.size]}
            for id, node in self.allocation.items()
        }
        return result

    def to_list(self) -> list:
        result = []
        for id, node in self.allocation.items():
            start_pos = node.start
            size = node.size
            end_pos = start_pos + size - 1
            value_list = [str(self.heap[i]) for i in range(start_pos, end_pos + 1)]
            result.append(value_list)
        return result
