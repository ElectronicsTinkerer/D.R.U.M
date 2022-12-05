

import sys, os

args = sys.argv[1:]

c_arr = ""

j = 0
file_size = 0

for arg in args:

    j += 1

    if file_size == 0:
        file_size = os.path.getsize(arg)
        c_arr += f"#define SAMPLE_LENGTH {int(file_size/int(2))}\n"
        c_arr += f"static uint16_t samples[][{file_size}] = {{\n"
    elif file_size != os.path.getsize(arg):
        print("ERROR: files are not the same size")
        exit(-1)
        
    with open(arg, "rb") as file:

        c_arr += "{\n"

        i = 0
        
        byte = file.read(2)
        while byte:
            i += 1
            c_arr += "0x"
            c_arr += byte[::-1].hex()
            byte = file.read(2)
            if byte:
                c_arr += ", "
            if i % 16 == 0:
                c_arr += "\n"
        
        c_arr += "}"

        if j < len(args):
            c_arr += ","
        c_arr += "\n"


c_arr += "};\n"


with open("samples.h", "w") as file:
    file.write(c_arr)

    

