import os

line = 0
for root, dir, file in os.walk('.'):
        for ff in file:
            with open(os.path.join(root, ff)) as f:
                line += len(f.readlines())
print(line)