// #include <pybind11/pybind11.h>
// #include "tmc2209_driver.h"

// namespace py = pybind11;

// PYBIND11_MODULE(tmc2209_module, m) {
//     m.doc() = "TMC2209 Linux driver";
//     py::class_<TMC2209Driver>(m, "TMC2209Driver")
//         .def(py::init<>());
// }
// 03/25/2026

// #include <pybind11/pybind11.h>
// #include "tmc2209_driver.h"

// namespace py = pybind11;

// PYBIND11_MODULE(tmc2209_module, m) {
//     m.doc() = "TMC2209 Linux driver";
//     py::class_<TMC2209Driver>(m, "TMC2209Driver")
//         .def(py::init<int, int, int, const char*>(),
//              py::arg("step_pin"),
//              py::arg("dir_pin"),
//              py::arg("en_pin"),
//              py::arg("uart_dev") = "/dev/ttyAMA2")
//         .def("configure", &TMC2209Driver::configure,
//              py::arg("current_ma"),
//              py::arg("microsteps"))
//         .def("set_enabled", &TMC2209Driver::setEnabled,
//              py::arg("en"));
// } 4/3/2026
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
             py::arg("en"))
        .def("step_pulse", [](TMC2209Driver& self, uint32_t steps, bool dir) {
            py::gil_scoped_release release;
            self.stepPulse(steps, dir);
        }, py::arg("steps"), py::arg("dir"))
        .def_readwrite("max_speed", &TMC2209Driver::max_speed_)
        .def_readwrite("accel", &TMC2209Driver::accel_)
        .def_readwrite("start_speed", &TMC2209Driver::start_speed_);
}
