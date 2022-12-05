

import sys

args = sys.argv[1:]

c_arr = ""
c_arr += "int16_t samples[][] = {\n"

j = 0

for arg in args:

    j += 1
    
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


c_arr += "}\n"


with open("samples.h", "w") as file:
    file.write(c_arr)

    

