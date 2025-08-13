import re
import sys

words = 0
with open(sys.argv[1], "rb") as f:
    pattern = rb"[^ \n\r\t\v\f]+"
    data = f.read()
    words = sum(1 for _ in re.finditer(pattern, data))

print(words)
