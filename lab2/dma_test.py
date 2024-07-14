import time 
import json
from temp import DMA

def my_load(dma_test, workload):
    assert_list = []
    size_list = []
    # print_list = None
    for request in workload:
        if request[0] == 'malloc':
            assert_list.append(dma_test.malloc(request[1], request[2], request[3]))
            # size_list.append(request[2])
        elif request[0] == 'free':
            assert_list.append(dma_test.free(request[1]))
        else:
            assert_list.append(dma_test.end())
    print_list = dma_test.to_list()
    return assert_list, print_list # , size_list

# workload_examples.json
with open('workload_examples.json', 'r') as file:
    data = json.load(file)

start_time = time.time()

for key in data:
    dma_test = None  
    workload = None
    id = None
    assert_list = None
    value_list = []
    for key2 in data[key]:
        if key2 == 'id' and key in data and key2 in data[key]:
            id = data[key][key2]
        if key2 == 'size' and key in data and key2 in data[key]:
            # dma_test = SmartDMA(data[key][key2])
            dma_test = DMA(data[key][key2])
        if key2 == 'sequence' and key in data and key2 in data[key]:
            workload = data[key][key2]
        if key2 == 'assert' and key in data and key2 in data[key]:
            assert_list = data[key][key2]
        if key2 == 'result' and key in data and key2 in data[key]:
            for key3 in data[key][key2]:
                value_list.append(key3['value'])
    
    if dma_test is not None and workload is not None:
        # print('workload:',id,'------------------------------------')
        list_1, list_2 = my_load(dma_test, workload)
        # print(list_1,list_2)
        # print()
        # print(size_list)
        if sorted(assert_list) == sorted(list_1):
            # print("True assert!")
            1
        else:
            assert("False assert!")
        
        if sorted(list_2) == sorted(value_list):
            # print("True value!")
            1
        else:
            assert("False value!")

end_time = time.time()  
run_time = end_time - start_time  
print("running time:", run_time, "s")

import resource

def memory_usage():
    usage = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    usage_mb = usage / 1024
    return usage_mb

print("Memory use：{} MB".format(memory_usage()))