#include "bindings.hpp"

#include "utils/pybind11_glm.hpp"

#include <iostream>
#include <pybind11/embed.h>

namespace festi {

void FestiBindings::init(py::module_& m) {
    m.doc() = "Python bindings for festi";

    py::class_<FestiModel::Transform>(m, "Transform")
        .def(py::init<>())
        .def_readwrite("translation", &FestiModel::Transform::translation)
        .def_readwrite("scale", &FestiModel::Transform::scale)
        .def_readwrite("rotation", &FestiModel::Transform::rotation)
        .def("get_model_matrix", &FestiModel::Transform::getModelMatrix)
        .def("get_normal_matrix", &FestiModel::Transform::getNormalMatrix)
        .def("random_offset", &FestiModel::Transform::randomOffset,
             py::arg("minOff"), py::arg("maxOff"), py::arg("basis"), py::arg("gen"))
        .def("__eq__", &FestiModel::Transform::operator==)
        .def("__ne__", &FestiModel::Transform::operator!=);

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

    py::class_<FestiModel::AsInstanceData>(m, "AsInstanceData")
        .def(py::init<>())
        .def_readwrite("random", &FestiModel::AsInstanceData::random)
        .def_readwrite("building", &FestiModel::AsInstanceData::building)
        .def_readwrite("layers", &FestiModel::AsInstanceData::layers)
        .def_readwrite("layerSeparation", &FestiModel::AsInstanceData::layerSeparation)
        .def("make_stand_alone", &FestiModel::AsInstanceData::makeStandAlone)
        .def("__eq__", &FestiModel::AsInstanceData::operator==)
        .def("__ne__", &FestiModel::AsInstanceData::operator!=);

    py::class_<FestiModel>(m, "Model")
        .def_static("createPointLight", 
            [this](float radius, glm::vec4 color) {
                return FestiModel::createPointLight(*FestiBindings::festiDevice, *FestiBindings::gameObjects, radius, color);
            },
        py::arg("radius"), py::arg("color"),
        "Creates a point light")
        .def_static("createModelFromFile",
            [this](const std::string& filepath, const std::string& mtlDir, const std::string& imgDir) {
                return FestiModel::createModelFromFile(*FestiBindings::festiDevice, *FestiBindings::festiMaterials, *FestiBindings::gameObjects, filepath, mtlDir, imgDir);
            },
            py::arg("filepath"), py::arg("mtlDir"), py::arg("imgDir"),
            "Creates a FestiModel object from a file, automatically using default settings.")
        .def("insert_keyframe", &FestiModel::insertKeyframe,
             py::arg("idx"), py::arg("flags"), py::arg("faceIDs") = std::vector<uint32_t>{0},
             "Insert a keyframe at the specified index")
        .def_readwrite("transform", &FestiModel::transform)
        .def_readwrite("as_instance_data", &FestiModel::asInstanceData)
        .def_readwrite("face_data", &FestiModel::faceData)
        .def_readwrite("visibility", &FestiModel::visibility)
        .def("get_id", &FestiModel::getId)
        .def("get_material", &FestiModel::getMaterial)
        .def("get_number_of_faces", &FestiModel::getNumberOfFaces)
        .def("get_shape_area", &FestiModel::getShapeArea)
        .def("all_faces", &FestiModel::ALL_FACES);
}

FestiDevice* FestiBindings::festiDevice = nullptr;
FestiMaterials* FestiBindings::festiMaterials = nullptr;
FS_ModelMap* FestiBindings::gameObjects = nullptr;

} // namespace festi

PYBIND11_EMBEDDED_MODULE(festi, m) {
    festi::FestiBindings bindings{};
    bindings.init(m);
}
