import random

N = 2000
with open("big_input.txt", "w") as f:
    f.write(f"{N}\n")
    for _ in range(N):
        m = random.uniform(1e20, 1e25)
        x = random.uniform(-1e12, 1e12)
        y = random.uniform(-1e12, 1e12)
        z = random.uniform(-1e12, 1e12)
        vx = random.uniform(-10000, 10000)
        vy = random.uniform(-10000, 10000)
        vz = random.uniform(-10000, 10000)
        f.write(f"{m} {x} {y} {z} {vx} {vy} {vz}\n")    