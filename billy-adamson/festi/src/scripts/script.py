import festi as fs
import numpy as np

objPath = "models/BUILDINGS/"
mtlPath = "models/BUILDINGS"
matPath = "materials/BUILDINGS"

# model = fs.Model.createModelFromFile(objPath + "mask.obj", mtlPath, matPath)

light = fs.Model.createPointLight(1, [1,1,1,1])

transform = fs.Transform()
lolz = fs.AsInstanceData()
j = transform.translation
