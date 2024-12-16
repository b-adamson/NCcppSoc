import festi as fs
import numpy as np

objPath = "models/BUILDINGS/"
mtlPath = "models/BUILDINGS"
matPath = "materials/BUILDINGS"

# model = fs.Model.createModelFromFile(objPath + "mask.obj", mtlPath, matPath)

transform = fs.Transform

transform.translation = "eeee"


lol = fs.Model

print(lol.translation)

# lol = fs.Model.createPointLight(1, [0,0,0,1])
