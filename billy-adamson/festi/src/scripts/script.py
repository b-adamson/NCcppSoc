import festi as fs
import numpy as np

objPath = "models/BUILDINGS/"
mtlPath = "models/BUILDINGS"
matPath = "materials/BUILDINGS"

model = fs.Model.createModelFromFile(objPath + "mask.obj", mtlPath, matPath)

for j in range(0, 100):
    model.transform.rotation[2] += 0.01
    # model.insertKeyframe(j, fs.FS_KEYFRAME_POS_ROT_SCALE)
