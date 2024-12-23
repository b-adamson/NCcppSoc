import festi as fs
import random as rnd
import math

SCENE_LENGTH = 300

objPath = "models/BUILDINGS/"
mtlPath = "models/BUILDINGS/"
matPath = "materials/BUILDINGS/"

# Set world properties
fs.scene.world.mainLightColour = [255.0 / 255.0, 255.0 / 255.0, 255.0 / 255.0, 0.4]
fs.scene.world.ambientColour = [1.0, 1.0, 1.0, 0.1]
fs.scene.world.mainLightDirection = [0.6, 1.4]
fs.scene.world.lightClip = [-30.0, 50.0]
fs.scene.world.cameraPosition = [-23.0, 11.0, -23.0]
fs.scene.world.cameraRotation = [-0.5, math.pi / 4, 0.0]
fs.scene.world.fov = math.radians(25)
fs.scene.world.clip = [0.1, 1000]
fs.scene.insertKeyframe(0, fs.KEYFRAME.WORLD)

# Create models
mask = fs.Model.createModelFromFile(objPath + "mask.obj", mtlPath, matPath)
mask.transform.scale = [0.3, 0.1, 0.6]
mask.transform.translation[2] += 20.0
mask.visibility = False
mask.insertKeyframe(0, fs.KEYFRAME.POS_ROT_SCALE | fs.KEYFRAME.VISIBILITY)

mask1 = fs.Model.createModelFromFile(objPath + "mask.obj", mtlPath, matPath)
mask1.transform.scale = [0.3, 0.1, 0.6]
mask1.transform.translation[0] += 20.0
mask1.transform.rotation[1] += math.pi / 2
mask1.visibility = False
mask1.insertKeyframe(0, fs.KEYFRAME.POS_ROT_SCALE | fs.KEYFRAME.VISIBILITY)

floor = fs.Model.createModelFromFile(objPath + "base.obj", mtlPath, matPath)
floor.transform.scale = [1.0, 2.0, 0.3]
floor.transform.translation[1] += 2.4
floor.transform.translation[2] += 2.4
floor.insertKeyframe(0, fs.KEYFRAME.POS_ROT_SCALE)

floor1 = fs.Model.createModelFromFile(objPath + "base.obj", mtlPath, matPath)
floor1.transform.scale = [1.0, 2.0, 0.3]
floor1.transform.translation[1] += floor.transform.translation[1]
floor1.transform.translation[0] += 2.4
floor1.insertKeyframe(0, fs.KEYFRAME.POS_ROT_SCALE)

strut = fs.Model.createModelFromFile(objPath + "cube.obj", mtlPath, matPath)
strut.transform.scale = [0.5, 1.0, 3.0]
strut.insertKeyframe(0, fs.KEYFRAME.POS_ROT_SCALE)

strut1 = fs.Model.createModelFromFile(objPath + "cube.obj", mtlPath, matPath)
strut1.transform.scale = [0.5, 1.0, 3.0]
strut1.insertKeyframe(0, fs.KEYFRAME.POS_ROT_SCALE)

# Create instance data for floor and strut
floorInst1 , floorInst2 = fs.AsInstanceData(), fs.AsInstanceData()
floorInsts = [floorInst1, floorInst2]
for floorInst in floorInsts:
    floorInst.layers = 10
    floorInst.layerSeparation = 5
    floorInst.building.columnDensity = 1

strutInst1 , strutInst2 = fs.AsInstanceData(), fs.AsInstanceData()
insts = [strutInst1, strutInst2]
for inst in insts:
    inst.layers = 10
    inst.layerSeparation = 5
    inst.building.columnDensity = 30
    inst.building.strutsPerColumnRange = [4, 4]
    inst.building.maxStrutOffset.scale = [3.0, 5.0, 5.9]
    inst.building.minStrutOffset.scale = [3.0, 5.0, 5.9]
    inst.building.minStrutOffset.translation = [0.0, 0.0, -0.6]
    inst.building.maxStrutOffset.translation = [0.0, 0.0, -0.6]
    inst.building.minColumnOffset.scale = [1.0, 5.0, 5.9]
    inst.building.maxColumnOffset.scale = [1.0, 5.0, 5.9]
    inst.building.alignToEdgeIdx = 1

rnd.seed()

for f in range(SCENE_LENGTH):
    # Rotate the camera position and main light
    if f % 3 == 0:
        fs.scene.world.cameraPosition[1] += 0.27
        fs.scene.world.cameraRotation[0] += 0.009
        fs.scene.insertKeyframe(f, fs.KEYFRAME.WORLD)

    # Modify instance data for strut
    for inst in insts:
        inst.layerSeparation += 0.001
        inst.building.jengaFactor = 1 - abs(f - 150) / 150.0
        inst.building.columnDensity = int(10 + abs(f - 150) / 6.0)
        inst.building.maxColumnOffset.translation[0] = 0.8
        inst.building.strutsPerColumnRange[0] = (4 - abs(f - 150) / 150.0 * 4)
        inst.building.maxStrutOffset.scale[0] = (1 - abs(f - 150) / 150.0) * 15

    # Modify instance data for floor
    for floorInst in floorInsts:
        floorInst.layerSeparation += 0.001
        floorInst.building.maxColumnOffset.rotation[1] = 0.05
        floorInst.building.minColumnOffset.rotation[1] = -0.05
        floorInst.building.seed = int(rnd.random() * 100)

    # face data
    buildingObjs = [strut, strut1, floor, floor1]
    for j in range(strut.getNumberOfFaces()):
        for obj in buildingObjs:
            obj.faceData[j].contrast = 0.8 + rnd.random() * 0.65
            obj.faceData[j].uvOffset = [rnd.random(), rnd.random()]

    # insert keyframes
    strut.asInstanceData = strutInst1
    strut.asInstanceData.parentObject = mask
    strut.insertKeyframe(f, fs.KEYFRAME.AS_INSTANCE | fs.KEYFRAME.FACE_MATERIALS, strut.allFaces())

    strut1.asInstanceData = strutInst2
    strut1.asInstanceData.parentObject = mask1
    strut1.insertKeyframe(f, fs.KEYFRAME.AS_INSTANCE | fs.KEYFRAME.FACE_MATERIALS, strut1.allFaces())

    floor.asInstanceData = floorInst1
    floor.asInstanceData.parentObject = mask
    floor.insertKeyframe(f, fs.KEYFRAME.AS_INSTANCE | fs.KEYFRAME.FACE_MATERIALS, floor.allFaces())

    floor1.asInstanceData = floorInst2
    floor1.asInstanceData.parentObject = mask1
    floor1.insertKeyframe(f, fs.KEYFRAME.AS_INSTANCE | fs.KEYFRAME.FACE_MATERIALS, floor1.allFaces())
