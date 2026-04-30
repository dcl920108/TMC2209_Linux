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
             py::arg("microsteps"),
             py::arg("sgthrs") = 70,
             py::arg("tcoolthrs") = 0xFFFFF)
        .def("set_enabled", &TMC2209Driver::setEnabled,
             py::arg("en"))
        .def("step_pulse", [](TMC2209Driver& self, uint32_t steps, bool dir) {
             py::gil_scoped_release release;
             self.stepPulse(steps, dir);
        }, py::arg("steps"), py::arg("dir"))
        .def("step_pulse_until_triggered", [](TMC2209Driver& self, uint32_t max_steps, bool dir, uint32_t threshold, uint32_t warmup_steps) {
             py::gil_scoped_release release;
             return self.stepPulseUntilTriggered(max_steps, dir, threshold, warmup_steps);
        }, py::arg("max_steps"), py::arg("dir"), py::arg("threshold"), py::arg("warmup_steps") = 200)
        .def_readwrite("max_speed",   &TMC2209Driver::max_speed_)
        .def_readwrite("accel",       &TMC2209Driver::accel_)
        .def_readwrite("start_speed", &TMC2209Driver::start_speed_)
        .def("get_ifcnt", &TMC2209Driver::getInterfaceCount)
        .def("is_communicating", &TMC2209Driver::isCommunicating)
        .def("read_diag_level", &TMC2209Driver::readDiagLevel)
        .def("is_stealthchop_enabled", &TMC2209Driver::isStealthChopEnabled)
        .def("get_version", &TMC2209Driver::getVersion);
}
