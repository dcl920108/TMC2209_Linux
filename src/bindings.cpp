// #include <pybind11/pybind11.h>
// #include "tmc2209_driver.h"

// namespace py = pybind11;

// PYBIND11_MODULE(tmc2209_module, m) {
//     m.doc() = "TMC2209 Linux driver";
//     py::class_<TMC2209Driver>(m, "TMC2209Driver")
//         .def(py::init<>());
// }
// 03/25/2026

#include <pybind11/pybind11.h>
#include "tmc2209_driver.h"

namespace py = pybind11;

PYBIND11_MODULE(tmc2209_module, m) {
    m.doc() = "TMC2209 Linux driver";
    py::class_<TMC2209Driver>(m, "TMC2209Driver")
        .def(py::init<int, int, int, const char*>(),
             py::arg("step_pin"),
             py::arg("dir_pin"),
             py::arg("en_pin"),
             py::arg("uart_dev") = "/dev/ttyAMA2")
        .def("configure", &TMC2209Driver::configure,
             py::arg("current_ma"),
             py::arg("microsteps"))
        .def("set_enabled", &TMC2209Driver::setEnabled,
             py::arg("en"));
}
