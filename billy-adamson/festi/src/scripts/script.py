import festi as fs
import sys


script_dir = "src/scripts"  # The directory you added to sys.path in C++

# if script_dir in sys.path:
    # sys.path.remove(script_dir)
    # sys.path.remove("bin")
    # sys.path.remove("")
    # sys.path.remove("C:\\msys64\\mingw64\\lib\\python312.zip")
    # sys.path.remove("C:\\msys64\\mingw64\\lib\\python3.12\\lib-dynload")
    # sys.path.remove("C:/msys64/mingw64/lib/python3.12/site-packages")
    

print(sys.path)

import numpy as np

objPath = "models/BUILDINGS/"
mtlPath = "models/BUILDINGS"
matPath = "materials/BUILDINGS"

# model = fs.Model.createModelFromFile(objPath + "mask.obj", mtlPath, matPath)

# lol = fs.Model.createPointLight(1, [0,0,0,1])
