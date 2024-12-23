#include "bindings.hpp"

// lib
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/complex.h>

PYBIND11_MAKE_OPAQUE(std::vector<festi::ObjFaceData>);

namespace festi {

void FestiBindings::init(py::module_& m) {
    m.doc() = "Python bindings for festi";

    m.def("material", &FestiModel::getMaterial);

    py::enum_<KeyFrameFlags>(m, "KEYFRAME", py::arithmetic())
        .value("POS_ROT_SCALE", FS_KEYFRAME_POS_ROT_SCALE)
        .value("FACE_MATERIALS", FS_KEYFRAME_FACE_MATERIALS)
        .value("POINT_LIGHT", FS_KEYFRAME_POINT_LIGHT)
        .value("AS_INSTANCE", FS_KEYFRAME_AS_INSTANCE)
        .value("WORLD", FS_KEYFRAME_WORLD)
        .value("VISIBILITY", FS_KEYFRAME_VISIBILITY)
        .export_values();

    py::class_<Transform>(m, "Transform")
        .def(py::init<>())
        .def_readwrite("translation", &Transform::translation)
        .def_readwrite("scale", &Transform::scale)
        .def_readwrite("rotation", &Transform::rotation)
        .def("getModelMatrix", &Transform::getModelMatrix)
        .def("getNormalMatrix", &Transform::getNormalMatrix)
        .def("randomOffset", &Transform::randomOffset,
             py::arg("minOff"), py::arg("maxOff"), py::arg("basis"), py::arg("gen"))
        .def("__eq__", &Transform::operator==)
        .def("__ne__", &Transform::operator!=);

    py::class_<FestiModel::AsInstanceData::RandomInstancesSettings>(m, "RandomInstancesSettings")
        .def(py::init<>())
        .def_readwrite("density", &FestiModel::AsInstanceData::RandomInstancesSettings::density)
        .def_readwrite("seed", &FestiModel::AsInstanceData::RandomInstancesSettings::seed)
        .def_readwrite("randomness", &FestiModel::AsInstanceData::RandomInstancesSettings::randomness)
        .def_readwrite("solidity", &FestiModel::AsInstanceData::RandomInstancesSettings::solidity)
        .def_readwrite("minOffset", &FestiModel::AsInstanceData::RandomInstancesSettings::minOffset)
        .def_readwrite("maxOffset", &FestiModel::AsInstanceData::RandomInstancesSettings::maxOffset);

    py::class_<FestiModel::AsInstanceData::BuildingInstancesSettings>(m, "BuildingInstancesSettings")
        .def(py::init<>())
        .def_readwrite("alignToEdgeIdx", &FestiModel::AsInstanceData::BuildingInstancesSettings::alignToEdgeIdx)
        .def_readwrite("columnDensity", &FestiModel::AsInstanceData::BuildingInstancesSettings::columnDensity)
        .def_readwrite("maxColumnOffset", &FestiModel::AsInstanceData::BuildingInstancesSettings::maxColumnOffset)
        .def_readwrite("minColumnOffset", &FestiModel::AsInstanceData::BuildingInstancesSettings::minColumnOffset)
        .def_readwrite("maxStrutOffset", &FestiModel::AsInstanceData::BuildingInstancesSettings::maxStrutOffset)
        .def_readwrite("minStrutOffset", &FestiModel::AsInstanceData::BuildingInstancesSettings::minStrutOffset)
        .def_readwrite("strutsPerColumnRange", &FestiModel::AsInstanceData::BuildingInstancesSettings::strutsPerColumnRange)
        .def_readwrite("jengaFactor", &FestiModel::AsInstanceData::BuildingInstancesSettings::jengaFactor)
        .def_readwrite("seed", &FestiModel::AsInstanceData::BuildingInstancesSettings::seed);

    py::bind_vector<std::vector<ObjFaceData>>(m, "test");
    py::class_<FestiModel::AsInstanceData>(m, "AsInstanceData")
        .def(py::init<>())
        .def_readwrite("parentObject", &FestiModel::AsInstanceData::parentObject)
        .def_readwrite("random", &FestiModel::AsInstanceData::random)
        .def_readwrite("building", &FestiModel::AsInstanceData::building)
        .def_readwrite("layers", &FestiModel::AsInstanceData::layers)
        .def_readwrite("layerSeparation", &FestiModel::AsInstanceData::layerSeparation)
        .def("make_stand_alone", &FestiModel::AsInstanceData::makeStandAlone)
        .def("__eq__", &FestiModel::AsInstanceData::operator==)
        .def("__ne__", &FestiModel::AsInstanceData::operator!=);

    py::class_<Material>(m, "Material")
        .def(py::init<>())
        .def_readwrite("diffuseColor", &Material::diffuseColor)
        .def_readwrite("specularColor", &Material::specularColor)
        .def_readwrite("shininess", &Material::shininess)
        .def_readwrite("diffuseTextureIndex", &Material::diffuseTextureIndex)
        .def_readwrite("specularTextureIndex", &Material::specularTextureIndex)
        .def_readwrite("normalTextureIndex", &Material::normalTextureIndex);

    py::class_<ObjFaceData, std::shared_ptr<ObjFaceData>>(m, "ObjFaceData")
        .def(py::init<>())
        .def_readwrite("materialID", &ObjFaceData::materialID)
        .def_readwrite("saturation", &ObjFaceData::saturation)
        .def_readwrite("contrast", &ObjFaceData::contrast)
        .def_readwrite("uvOffset", &ObjFaceData::uvOffset)
        .def("__eq__", &ObjFaceData::operator==)
        .def("__ne__", &ObjFaceData::operator!=);

    py::class_<FestiModel, std::shared_ptr<FestiModel>>(m, "Model")
        .def_static("createModelFromFile",
            [this](const std::string& filepath, const std::string& mtlDir, const std::string& imgDir) {
                return FestiModel::createModelFromFile(*festiDevice, *festiMaterials, *gameObjects, filepath, mtlDir, imgDir);
            },
            py::return_value_policy::reference,
            py::arg("filepath"), py::arg("mtlDir"), py::arg("imgDir"))
        .def("insertKeyframe", &FestiModel::insertKeyframe,
             py::arg("idx"), py::arg("flags"), py::arg("faceIDs") = std::vector<uint32_t>{0})
        .def_readwrite("transform", &FestiModel::transform)
        .def_readwrite("asInstanceData", &FestiModel::asInstanceData)
        .def_readwrite("faceData", &FestiModel::faceData, py::return_value_policy::reference_internal)
        .def_readwrite("visibility", &FestiModel::visibility)
        .def("getId", &FestiModel::getId)
        .def("getMaterial", &FestiModel::getMaterial)
        .def("getNumberOfFaces", &FestiModel::getNumberOfFaces)
        .def("getShapeArea", &FestiModel::getShapeArea)
        .def("allFaces", &FestiModel::ALL_FACES)
        .def("setFaces",
            &FestiModel::setFaces,
            py::arg("data"), py::arg("faces") = std::vector<uint32_t>{FS_UNSPECIFIED});
        

    py::class_<FestiPointLight, std::shared_ptr<FestiPointLight>>(m, "PointLight")
        .def_static("createPointLight", 
            [this](float radius, glm::vec4 color) {
                return FestiPointLight::createPointLight(*pointLights, radius, color);
            },
            py::return_value_policy::reference,
            py::arg("radius"), py::arg("color"))
        .def("insertKeyframe", &FestiPointLight::insertKeyframe,
             py::arg("idx"), py::arg("flags"))
        .def_readwrite("visibility", &FestiPointLight::visibility)
        .def_readwrite("transform", &FestiPointLight::transform)
        .def("getId", &FestiPointLight::getId);

    py::class_<FestiWorld::WorldProperties>(m, "WorldProperties")
        .def_readwrite("mainLightColour", &FestiWorld::WorldProperties::mainLightColour)
        .def_readwrite("mainLightDirection", &FestiWorld::WorldProperties::mainLightDirection)
        .def_readwrite("ambientColour", &FestiWorld::WorldProperties::ambientColour)
        .def_readwrite("clipDist", &FestiWorld::WorldProperties::clipDist)
        .def_readwrite("cameraPosition", &FestiWorld::WorldProperties::cameraPosition)
        .def_readwrite("cameraRotation", &FestiWorld::WorldProperties::cameraRotation)
        .def("getDirectionVector", &FestiWorld::WorldProperties::getDirectionVector);

    py::class_<FestiWorld, std::shared_ptr<FestiWorld>>(m, "FestiWorld")
        .def("insertKeyframe", &FestiWorld::insertKeyframe)
        .def_readwrite("world", &FestiWorld::world);

    m.attr("scene") = py::cast(*scene); 
}

FestiDevice* FestiBindings::festiDevice = nullptr;
FestiMaterials* FestiBindings::festiMaterials = nullptr;
FS_ModelMap* FestiBindings::gameObjects = nullptr;
FS_PointLightMap* FestiBindings::pointLights = nullptr;
FS_World* FestiBindings::scene = nullptr;

} // festi

PYBIND11_EMBEDDED_MODULE(festi, m) {
    festi::FestiBindings bindings{};
    bindings.init(m);
}
