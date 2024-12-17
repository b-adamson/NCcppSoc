import festi as fs
import numpy as np

objPath = "models/BUILDINGS/"
mtlPath = "models/BUILDINGS"
matPath = "materials/BUILDINGS"

model = fs.Model.createModelFromFile(objPath + "mask.obj", mtlPath, matPath)
