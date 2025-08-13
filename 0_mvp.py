import sys

ws = set(b" \n\r\t\v\f")
prev_ws = True
words = 0
with open(sys.argv[1], "rb") as f:
    for byte_value in f.read():
        cur_ws = byte_value in ws
        if not cur_ws and prev_ws:
            words += 1
        prev_ws = cur_ws

print(words)
