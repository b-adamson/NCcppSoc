import festi as fs
import numpy as np

objPath = "models/BUILDINGS/"
mtlPath = "models/BUILDINGS"
matPath = "materials/BUILDINGS"

model = fs.Model.createModelFromFile(objPath + "mask.obj", mtlPath, matPath)

light = fs.PointLight.createPointLight(4, [.2,.2,1,1])

for f in range(0, 300):
    model.transform.rotation += [0, 0, 0.01]
    model.insertKeyframe(f, fs.KEYFRAME.POS_ROT_SCALE)

    print(model.transform.rotation)
