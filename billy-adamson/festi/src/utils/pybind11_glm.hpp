#ifndef GLM_NUMPY_CAST_HPP
#define GLM_NUMPY_CAST_HPP

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <glm/glm.hpp>
#include <vector>

namespace py = pybind11;

namespace pybind11 { namespace detail {

// Custom type caster for glm::vec2 -> numpy array and vice versa
template <> struct type_caster<glm::vec2> {
public:
    PYBIND11_TYPE_CASTER(glm::vec2, const_name("glm.vec2"));

    // Conversion from Python -> C++ (load)
    bool load(handle src, bool) {
        // Ensure the handle is a NumPy array
        py::array arr = py::array::ensure(src);
        if (!arr) return false;

        // Ensure the array is 1D with exactly 2 elements
        if (arr.ndim() != 1 || arr.shape(0) != 2) return false;

        // Copy data into glm::vec2
        auto r = arr.cast<py::array_t<double>>().unchecked<1>();
        value = glm::vec2(r(0), r(1));
        return true;
    }

    // Conversion from C++ -> Python (cast)
    static handle cast(const glm::vec2& src, return_value_policy, handle) {
        // Allocate independent memory for the numpy array
        std::vector<double> data = {src.x, src.y};
        return py::array_t<double>(
            {2},                // Shape: 2 elements for vec2
            {sizeof(double)},   // Strides: distance between elements
            data.data()         // Pointer to data
        ).release();
    }
};

// Custom type caster for glm::vec3 -> numpy array and vice versa
template <> struct type_caster<glm::vec3> {
public:
    PYBIND11_TYPE_CASTER(glm::vec3, const_name("glm.vec3"));

    // Conversion from Python -> C++ (load)
    bool load(handle src, bool) {
        // Ensure the handle is a NumPy array
        py::array arr = py::array::ensure(src);
        if (!arr) return false;

        // Ensure the array is 1D with exactly 3 elements
        if (arr.ndim() != 1 || arr.shape(0) != 3) return false;

        // Copy data into glm::vec3
        auto r = arr.cast<py::array_t<double>>().unchecked<1>();
        value = glm::vec3(r(0), r(1), r(2));
        return true;
    }

    // Conversion from C++ -> Python (cast)
    static handle cast(const glm::vec3& src, return_value_policy, handle) {
        // Allocate independent memory for the numpy array
        std::vector<double> data = {src.x, src.y, src.z};
        return py::array_t<double>(
            {3},                // Shape: 3 elements for vec3
            {sizeof(double)},   // Strides: distance between elements
            data.data()         // Pointer to data
        ).release();
    }
};

// Custom type caster for glm::vec4 -> numpy array and vice versa
template <> struct type_caster<glm::vec4> {
public:
    PYBIND11_TYPE_CASTER(glm::vec4, const_name("glm.vec4"));

    // Conversion from Python -> C++ (load)
    bool load(handle src, bool) {
        // Ensure the handle is a NumPy array
        py::array arr = py::array::ensure(src);
        if (!arr) return false;

        // Ensure the array is 1D with exactly 4 elements
        if (arr.ndim() != 1 || arr.shape(0) != 4) return false;

        // Copy data into glm::vec4
        auto r = arr.cast<py::array_t<double>>().unchecked<1>();
        value = glm::vec4(r(0), r(1), r(2), r(3));
        return true;
    }

    // Conversion from C++ -> Python (cast)
    static handle cast(const glm::vec4& src, return_value_policy, handle) {
        // Allocate independent memory for the numpy array
        std::vector<double> data = {src.x, src.y, src.z, src.w};
        return py::array_t<double>(
            {4},                // Shape: 4 elements for vec4
            {sizeof(double)},   // Strides: distance between elements
            data.data()         // Pointer to data
        ).release();
    }
};

// Custom type caster for glm::mat4 -> numpy array and vice versa
template <> struct type_caster<glm::mat4> {
public:
    PYBIND11_TYPE_CASTER(glm::mat4, const_name("glm.mat4"));

    // Conversion from Python -> C++ (load)
    bool load(handle src, bool) {
        // Ensure the handle is a NumPy array
        py::array arr = py::array::ensure(src);
        if (!arr) return false;

        // Ensure the array is 2D with exactly 4x4 elements
        if (arr.ndim() != 2 || arr.shape(0) != 4 || arr.shape(1) != 4) return false;

        // Copy data into glm::mat4
        auto r = arr.cast<py::array_t<double>>().unchecked<2>();
        glm::mat4 result;
        for (size_t i = 0; i < 4; ++i) {
            for (size_t j = 0; j < 4; ++j) {
                result[i][j] = r(i, j);
            }
        }
        value = result;
        return true;
    }

    // Conversion from C++ -> Python (cast)
    static handle cast(const glm::mat4& src, return_value_policy, handle) {
        // Allocate independent memory for the numpy array
        std::vector<double> data(16);
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                data[i * 4 + j] = src[i][j];
            }
        }
        return py::array_t<double>(
            {4, 4},                 // Shape: 4x4 elements for mat4
            {sizeof(double) * 4,    // Strides: distance between rows
             sizeof(double)},       // Strides: distance between elements
            data.data()             // Pointer to data
        ).release();
    }
};

}} // namespace pybind11::detail

#endif // GLM_NUMPY_CAST_HPP
