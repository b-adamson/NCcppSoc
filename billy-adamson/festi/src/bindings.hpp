#pragma once

#include "model.hpp"

// lib
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/functional.h>

namespace festi {
namespace py = pybind11;

class FestiBindings {
public:
    FestiBindings(FestiDevice& festiDevice, FestiMaterials& festiMaterials, FS_ModelMap& gameObjects);
    void init(py::module_& m);

private:
    FestiDevice& festiDevice;     
    FestiMaterials& festiMaterials;
    FS_ModelMap& gameObjects;
};

} // festi
