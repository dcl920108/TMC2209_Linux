#include <pybind11/pybind11.h>
#include "tmc2209_driver.h"

namespace py = pybind11;

PYBIND11_MODULE(tmc2209_module, m) {
    m.doc() = "TMC2209 Linux driver";
    py::class_<TMC2209Driver>(m, "TMC2209Driver")
        .def(py::init<>());
}
