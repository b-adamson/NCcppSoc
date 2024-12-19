import festi as fs
import random
import math
import numpy as np

SCENE_LENGTH = 300

objPath = "models/WALLROTATING/"
mtlPath = "models/WALLROTATING/"
matPath = "materials/WALLROTATING/"

# Create models
floor = fs.Model.createModelFromFile(objPath + "floor.obj", mtlPath, matPath)
floor.transform.scale = [7.0, 3.0, 7.0]

wall = fs.Model.createModelFromFile(objPath + "wall.obj", mtlPath, matPath)
wall.transform.scale = [0.1, 0.1, 0.2]

door = fs.Model.createModelFromFile(objPath + "door.obj", mtlPath, matPath)
door.transform.translation = [0.0, 0.95, 0.0]
door.transform.scale = [0.05, 0.12, 0.12]

arch = fs.Model.createModelFromFile(objPath + "arch.obj", mtlPath, matPath)
arch.transform.translation = [0.0, 2.0, 0.0]

mask = fs.Model.createModelFromFile(objPath + "mask.obj", mtlPath, matPath)
mask.transform.scale = [0.1, 0.1, 0.2]
mask.transform.rotation[1] = math.pi
mask.transform.translation[2] += 0.1
mask.visibility = False
mask.insertKeyframe(0, fs.KEYFRAME.VISIBILITY)

lamp = fs.Model.createModelFromFile(objPath + "lamp.obj", mtlPath, matPath)
lamp.transform.scale = [0.1, 0.1, 0.1]

box = fs.Model.createModelFromFile(objPath + "box.obj", mtlPath, matPath)
box.transform.translation = [0.0, -3.0, -1.5]
box.transform.scale = [3.0, 3.0, 3.0]

leaf = fs.Model.createModelFromFile(objPath + "leaf.obj", mtlPath, matPath)
leaf.transform.translation[2] = 0.7
leaf.insertKeyframe(0, fs.KEYFRAME.POS_ROT_SCALE)

fs.ObjFaceData()

# Set leaf face data
leafData = fs.ObjFaceData()
leafData.saturation = 0.0
leafData.contrast = 2.0
leaf.setFaces(leafData)
leaf.insertKeyframe(0, fs.KEYFRAME.FACE_MATERIALS, leaf.allFaces())

streetLight = fs.Model.createModelFromFile(objPath + "streetLight.obj", mtlPath, matPath)
streetLight.transform.rotation = [math.pi, 0.0, 0.0]
streetLight.transform.scale = [0.3, 0.2, 0.3]

# Create point lights (flames)
translations = [
    [-2.9, 2.6, -0.8],
    [-1.6, 2.6, -0.8],
    [1.6, 2.6, -0.8],
    [2.9, 2.6, -0.8],
]

flames = []
for i in range(9):
    flames.append(fs.PointLight.createPointLight(0.5, [1.0, 1.0, 1.0, 1.0]))

for i, t in enumerate(translations):
    flames[i].transform.translation = t

# Initialize world properties
fs.scene.world.mainLightColour = [255.0/255.0, 255.0/255.0, 255.0/255.0, 1.0]
fs.scene.world.ambientColour = [1.0, 1.0, 1.0, 0.01]
fs.scene.world.mainLightDirection = [0.4, 0.0]
fs.scene.insertKeyframe(0, fs.KEYFRAME.WORLD)

random.seed()

for f in range(SCENE_LENGTH):
    frameRND = random.random()

    # floor keyframes
    floor.transform.rotation[1] += math.pi / 150.0
    floor.insertKeyframe(f, fs.KEYFRAME.POS_ROT_SCALE)

    # arch keyframes
    arch.transform.rotation[1] += math.pi / 150.0
    arch.transform.scale = [
        frameRND * 0.01 + 0.145,
        0.15 - (SCENE_LENGTH / 3000.0) + (f / 3000.0),
        frameRND + 0.5
    ]
    arch.transform.scale[1] += (frameRND - 0.5) * 0.1
    arch.insertKeyframe(f, fs.KEYFRAME.POS_ROT_SCALE)

    # door keyframes
    door.transform.rotation[1] = math.pi * f / 150.0 + math.pi / 2.0
    door.transform.scale[1] = (frameRND - 0.5) * 0.01 + 0.13
    door.insertKeyframe(f, fs.KEYFRAME.FACE_MATERIALS, [0, 2, 6, 8])
    door.insertKeyframe(f, fs.KEYFRAME.POS_ROT_SCALE)

    # wall keyframes
    wall.transform.rotation[1] += math.pi / 150.0
    wall.transform.scale[1] = (frameRND - 0.5) * 0.01 + 0.1
    wall.insertKeyframe(f, fs.KEYFRAME.POS_ROT_SCALE)

    # mask keyframes
    mask.transform.rotation[1] += math.pi / 150.0
    mask.transform.scale[0] = (frameRND - 0.5)*0.05 + 0.1
    mask.insertKeyframe(f, fs.KEYFRAME.POS_ROT_SCALE)

#     # box keyframes
#     box.transform.translation[1] += 0.007
#     box.asInstanceData.parentObject = mask
#     box.asInstanceData.random.seed = f
#     box.asInstanceData.random.density = random.random() / 10.0
#     box.asInstanceData.random.minOffset.scale = [0.4, 0.8, 0.3]
#     box.asInstanceData.random.maxOffset.scale = [1.7, 2.3, 2.5]
#     box.insertKeyframe(f, fs.KEYFRAME.AS_INSTANCE | fs.KEYFRAME.POS_ROT_SCALE)

#     # leaf keyframes
#     leaf.asInstanceData.parentObject = mask
#     leaf.asInstanceData.random.density = 2.0
#     leaf.asInstanceData.random.seed = f
#     leaf.asInstanceData.random.minOffset.translation[0] = -1.0
#     leaf.asInstanceData.random.maxOffset.translation[0] = 1.0
#     leaf.insertKeyframe(f, fs.KEYFRAME.AS_INSTANCE)

#     # lamp keyframes
#     lamp.transform.rotation[1] = math.pi * f / 150.0 + math.pi
#     lamp.transform.scale[1] = (frameRND - 0.5)*0.01 + 0.1
#     lamp.transform.translation[1] -= 0.005
#     lamp.insertKeyframe(f, fs.KEYFRAME.POS_ROT_SCALE)

#     # streetLight keyframes
#     angle = f * math.pi / 150.0
#     oldPos = [frameRND, frameRND - 0.5, 1.5 + frameRND]
#     ca = math.cos(angle)
#     sa = math.sin(angle)
#     newPos = [oldPos[0]*ca + oldPos[2]*sa, oldPos[1], -oldPos[0]*sa + oldPos[2]*ca]
#     streetLight.transform.translation = newPos
#     streetLight.transform.rotation[1] += math.pi / 150.0
#     streetLight.insertKeyframe(f, fs.KEYFRAME.POS_ROT_SCALE)

#     # flames[0..3] keyframes
#     angle2 = math.pi / 150.0
#     c2 = math.cos(angle2)
#     s2 = math.sin(angle2)
#     for i in range(4):
#         pos = flames[i].transform.translation
#         rotated = [pos[0]*c2 + pos[2]*s2, pos[1], -pos[0]*s2 + pos[2]*c2]
#         flames[i].transform.translation = rotated
#         flames[i].transform.translation[1] -= 0.005
#         flames[i].transform.translation[2] += (random.random()-0.5)*0.04
#         if int(random.random()*4) == 0:
#             flames[i].visibility = not flames[i].visibility
#         flames[i].insertKeyframe(f, fs.KEYFRAME.POS_ROT_SCALE | fs.KEYFRAME.VISIBILITY)

#     # flames[4..8] keyframes
#     xcoords = [0.0, -2.5, 2.5, -5.0, 5.0]
#     for i in range(4, 9):
#         angle3 = f * math.pi / 150.0
#         c3 = math.cos(angle3)
#         s3 = math.sin(angle3)
#         basePos = [frameRND + xcoords[i-4], frameRND + 2.8, 1.3 + frameRND]
#         newPos2 = [basePos[0]*c3 + basePos[2]*s3, basePos[1], -basePos[0]*s3 + basePos[2]*c3]
#         flames[i].transform.translation = newPos2
#         flames[i].insertKeyframe(f, fs.KEYFRAME.POS_ROT_SCALE)

#     # floor face data
#     num_floor_faces = floor.getNumberOfFaces()
#     for j in range(num_floor_faces):
#         floor.faceData[j].uvOffset = [random.random(), random.random()]
#         floor.faceData[j].contrast = 5.0
#     floor.insertKeyframe(f, fs.KEYFRAME.FACE_MATERIALS, floor.allFaces())

#     # box face data
#     num_box_faces = box.getNumberOfFaces()
#     for j in range(num_box_faces):
#         box.faceData[j].saturation = 0.1
#         box.faceData[j].contrast = 1.6
#     box.insertKeyframe(f, fs.KEYFRAME.FACE_MATERIALS, box.allFaces())

#     # wall face data
#     num_wall_faces = wall.getNumberOfFaces()
#     for j in range(num_wall_faces):
#         wall.faceData[j].contrast = (random.random()-0.2)*2 + 4.0
#         wall.faceData[j].uvOffset = [random.random(), random.random()]
#         wall.faceData[j].saturation = 0.0
#     wall.insertKeyframe(f, fs.KEYFRAME.FACE_MATERIALS, wall.allFaces())

#     # arch face data
#     num_arch_faces = arch.getNumberOfFaces()
#     for i in range(num_arch_faces):
#         uvOffset = [random.random(), random.random()]
#         arch.faceData[i].uvOffset = uvOffset
#         arch.faceData[i].contrast = (random.random()-0.2)*1.1 + 16.0
#     arch.insertKeyframe(f, fs.KEYFRAME.FACE_MATERIALS, arch.allFaces())

#     # door face data
#     offsetX = int(random.random()*8)/8.0
#     offsetY = int(random.random()*5)/5.0
#     offset = [offsetX, offsetY]
#     for face in [0, 2, 6, 8]:
#         door.faceData[face].uvOffset = offset
#         door.faceData[face].saturation = 2.0
#     door.insertKeyframe(f, fs.KEYFRAME.FACE_MATERIALS, [0, 2, 6, 8])

#     # streetLight face data
#     num_streetLight_faces = streetLight.getNumberOfFaces()
#     for i in range(num_streetLight_faces):
#         streetLight.faceData[i].contrast = 1.5
#     streetLight.insertKeyframe(f, fs.KEYFRAME.FACE_MATERIALS, streetLight.allFaces())
