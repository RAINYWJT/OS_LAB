import random

cnt = 0

for _ in range(10000):
    temp = [random.choice([0, 1]) for _ in range(7)]
    for i in range(5):
        if temp[i] == 1 and temp[i+1] == 1 and temp[i+2] == 1:
            cnt += 1
            break

print(cnt / 10000)