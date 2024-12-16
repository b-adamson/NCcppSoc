#pragma once

#include "model.hpp"

// lib
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/functional.h>
#include <glm/gtc/type_ptr.hpp>

namespace py = pybind11;

namespace festi {

class FestiBindings {
public:
    FestiBindings() {};
    void init(py::module_& m);

    static FestiDevice* festiDevice;
    static FestiMaterials* festiMaterials;
    static FS_ModelMap* gameObjects;
};

} // festi
