import festi as fs
import random as rnd
import math

SCENE_LENGTH = 300

objPath = "models/WALLROTATING/"
mtlPath = "models/WALLROTATING/"
matPath = "materials/WALLROTATING/"

# Create models
floor = fs.Model.createModelFromFile(objPath + "floor.obj", mtlPath, matPath)
floor.transform.scale = [7.0, 3.0, 7.0]
floor.insertKeyframe(0, fs.KEYFRAME.POS_ROT_SCALE)

wall = fs.Model.createModelFromFile(objPath + "wall.obj", mtlPath, matPath)
wall.transform.scale = [0.1, 0.1, 0.2]

door = fs.Model.createModelFromFile(objPath + "door.obj", mtlPath, matPath)
door.transform.translation = [0.0, 0.95, 0.0]
door.transform.rotation[1] = math.pi / 2.0
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
lamp.transform.rotation[1] = math.pi

box = fs.Model.createModelFromFile(objPath + "box.obj", mtlPath, matPath)
box.transform.translation = [0.0, -3.0, 1.5]
box.transform.scale = [3.0, 3.0, 3.0]

leaf = fs.Model.createModelFromFile(objPath + "leaf.obj", mtlPath, matPath)
leaf.transform.translation[2] = -0.8
leaf.insertKeyframe(0, fs.KEYFRAME.POS_ROT_SCALE)

streetLight = fs.Model.createModelFromFile(objPath + "streetLight.obj", mtlPath, matPath)
streetLight.transform.rotation = [math.pi, 0.0, 0.0]
streetLight.transform.scale = [0.3, 0.2, 0.3]

# Create point lights (flames)
translations = [[-2.9, 2.6, -0.8], [-1.6, 2.6, -0.8], [1.6, 2.6, -0.8], [2.9, 2.6, -0.8],]
flames = []
for i in range(9):
    flames.append(fs.PointLight.createPointLight(0.5, [1.0, 1.0, 1.0, 1.0]))
for i, t in enumerate(translations):
    flames[i].transform.translation = t

# Initialize world properties
fs.scene.world.mainLightColour = [255.0/255.0, 255.0/255.0, 255.0/255.0, 1.0]
fs.scene.world.ambientColour = [1.0, 1.0, 1.0, 0.01]
fs.scene.world.mainLightDirection = [0.4, 0.0]
fs.scene.world.cameraPosition = [5.0, 1.0, 0.0]
fs.scene.world.cameraRotation = [0.0, -math.pi/2, 0.0]
fs.scene.world.clip = [0.1, 1000]
fs.scene.world.fov = math.radians(40)

rnd.seed()

for f in range(SCENE_LENGTH):
    # Rotate the camera position and main light
    angle = -math.pi / 150.0
    c1, s2 = math.cos(angle), math.sin(angle)
    camera_pos = fs.scene.world.cameraPosition
    fs.scene.world.cameraPosition = [camera_pos[0] * c1 + camera_pos[2] * s2, camera_pos[1], -camera_pos[0] * s2 + camera_pos[2] * c1]
    fs.scene.world.mainLightDirection = [0.4, -f * math.pi / 150.0]
    fs.scene.world.cameraRotation[1] = -math.pi * f / 150.0 - math.pi / 2
    fs.scene.insertKeyframe(f, fs.KEYFRAME.WORLD)
    frameRND = rnd.random()

    arch.transform.scale = [frameRND * 0.01 + 0.145, 0.15 - (SCENE_LENGTH / 3000.0) + (f / 3000.0), frameRND + 0.5]
    arch.transform.scale[1] += (frameRND - 0.5) * 0.1
    arch.insertKeyframe(f, fs.KEYFRAME.POS_ROT_SCALE)

    door.transform.scale[1] = (frameRND - 0.5) * 0.01 + 0.13
    door.insertKeyframe(f, fs.KEYFRAME.FACE_MATERIALS | fs.POS_ROT_SCALE, [0, 2, 6, 8])

    wall.transform.scale[1] = (frameRND - 0.5) * 0.01 + 0.1
    wall.insertKeyframe(f, fs.KEYFRAME.POS_ROT_SCALE)

    mask.transform.scale[0] = (frameRND - 0.5) * 0.05 + 0.1
    mask.insertKeyframe(f, fs.KEYFRAME.POS_ROT_SCALE)

    box.transform.translation[1] += 0.007
    box.asInstanceData.parentObject = mask
    box.asInstanceData.random.seed = f
    box.asInstanceData.random.density = rnd.random() / 10.0
    box.asInstanceData.random.minOffset.scale = [0.4, 0.8, 0.3]
    box.asInstanceData.random.maxOffset.scale = [1.7, 2.3, 2.5]
    box.insertKeyframe(f, fs.KEYFRAME.POS_ROT_SCALE | fs.KEYFRAME.AS_INSTANCE)

    leaf.asInstanceData.parentObject = mask
    leaf.asInstanceData.random.density = 2.0
    leaf.asInstanceData.random.seed = f
    leaf.asInstanceData.random.minOffset.translation[0] = -1.0
    leaf.asInstanceData.random.maxOffset.translation[0] = 1.0
    leaf.insertKeyframe(f, fs.KEYFRAME.AS_INSTANCE)

    lamp.transform.rotation[1] = math.pi
    lamp.transform.scale[1] = (frameRND - 0.5)*0.01 + 0.1
    lamp.transform.translation[1] -= 0.005
    lamp.insertKeyframe(f, fs.KEYFRAME.POS_ROT_SCALE)

    streetLight.transform.translation = [frameRND, frameRND - 0.5, 1.5 + frameRND]
    streetLight.insertKeyframe(f, fs.KEYFRAME.POS_ROT_SCALE)

    # flames
    for i in range(4):
        flames[i].transform.translation[1] -= 0.005
        if int(rnd.random()*4) == 0:
            flames[i].visibility = not flames[i].visibility
        flames[i].insertKeyframe(f, fs.KEYFRAME.POS_ROT_SCALE | fs.KEYFRAME.VISIBILITY)

    xcoords = [0.0, -2.5, 2.5, -5.0, 5.0]
    for i in range(4, 9):
        flames[i].transform.translation = [frameRND + xcoords[i-4], frameRND + 2.8, 1.3 + frameRND]
        flames[i].insertKeyframe(f, fs.KEYFRAME.POS_ROT_SCALE)

    # face data
    floor.setFaces(fs.ObjFaceData(matID=fs.material("graybrick"), sat=1.0, con=5.0, uv=[rnd.random(), rnd.random()]))
    floor.insertKeyframe(f, fs.KEYFRAME.FACE_MATERIALS, floor.allFaces())

    box.setFaces(fs.ObjFaceData(matID=fs.material("wood"), sat=0.1, con=1.6, uv=[0.0, 0.0]))
    box.insertKeyframe(f, fs.KEYFRAME.FACE_MATERIALS, box.allFaces())

    wall.setFaces(fs.ObjFaceData(matID=fs.material("graybrick"), sat=0.0, con=(rnd.random() - 0.2) * 2 + 4.0, uv=[rnd.random(), rnd.random()]))
    wall.insertKeyframe(f, fs.KEYFRAME.FACE_MATERIALS, wall.allFaces())

    arch.setFaces(fs.ObjFaceData(matID=fs.material("graybrick"), sat=1.0, con=(rnd.random() - 0.2) * 1.1 + 16.0, uv=[rnd.random(), rnd.random()]))
    arch.insertKeyframe(f, fs.KEYFRAME.FACE_MATERIALS, arch.allFaces())

    door.setFaces(
        fs.ObjFaceData(matID=fs.material("doors"), sat=2.0, con=1.0, uv=[int(rnd.random() * 8) / 8.0, int(rnd.random() * 5) / 5.0]), [0, 2, 6, 8])
    door.insertKeyframe(f, fs.KEYFRAME.FACE_MATERIALS, [0, 2, 6, 8])

    streetLight.setFaces(fs.ObjFaceData(matID=fs.material("concrete"), sat=1.0, con=1.5, uv=[0.0, 0.0]))
    streetLight.insertKeyframe(f, fs.KEYFRAME.FACE_MATERIALS, streetLight.allFaces())

leaf.setFaces(fs.ObjFaceData(matID=fs.material("leaf"), sat=0.0, con=2.0, uv=[0, 0]))
leaf.insertKeyframe(0, fs.KEYFRAME.FACE_MATERIALS, leaf.allFaces())
