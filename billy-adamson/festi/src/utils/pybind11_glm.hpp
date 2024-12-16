#ifndef GLM_NUMPY_CAST_HPP
#define GLM_NUMPY_CAST_HPP

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <glm/glm.hpp>

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

        // Use .unchecked() to access the raw data as a double array
        py::array_t<double> arr_unchecked = arr.cast<py::array_t<double>>();
        auto r = arr_unchecked.unchecked<1>(); // 1D array
        value = glm::vec2(r(0), r(1));
        return true;
    }

    // Conversion from C++ -> Python (cast)
    static handle cast(const glm::vec2& src, return_value_policy, handle) {
        // Using py::array and correctly formatting buffer_info
        return py::array(py::buffer_info(
            (void*)&src,                // Pointer to data
            sizeof(double),             // Size of each element (double)
            py::format_descriptor<double>::format(), // Data type format string
            1,                          // Number of dimensions (1D array)
            {2},                      // Shape of the array (2 elements for vec2)
            {sizeof(double)}          // Strides (in bytes)
        ));
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

        // Use .unchecked() to access the raw data as a double array
        py::array_t<double> arr_unchecked = arr.cast<py::array_t<double>>();
        auto r = arr_unchecked.unchecked<1>(); // 1D array
        value = glm::vec3(r(0), r(1), r(2));
        return true;
    }

    // Conversion from C++ -> Python (cast)
    static handle cast(const glm::vec3& src, return_value_policy, handle) {
        return py::array(py::buffer_info(
            (void*)&src,                // Pointer to data
            sizeof(double),             // Size of each element (double)
            py::format_descriptor<double>::format(), // Data type format string
            1,                          // Number of dimensions (1D array)
            { 3 },                      // Shape of the array (3 elements for vec3)
            { sizeof(double) }          // Strides (in bytes)
        ));
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

        // Use .unchecked() to access the raw data as a double array
        py::array_t<double> arr_unchecked = arr.cast<py::array_t<double>>();
        auto r = arr_unchecked.unchecked<1>(); // 1D array
        value = glm::vec4(r(0), r(1), r(2), r(3));
        return true;
    }

    // Conversion from C++ -> Python (cast)
    static handle cast(const glm::vec4& src, return_value_policy, handle) {
        return py::array(py::buffer_info(
            (void*)&src,                // Pointer to data
            sizeof(double),             // Size of each element (double)
            py::format_descriptor<double>::format(), // Data type format string
            1,                          // Number of dimensions (1D array)
            { 4 },                      // Shape of the array (4 elements for vec4)
            { sizeof(double) }          // Strides (in bytes)
        ));
    }
};

}} // namespace pybind11::detail

#endif // GLM_NUMPY_CAST_HPP
