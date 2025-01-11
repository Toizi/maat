#include "python_bindings.hpp"

namespace maat{
namespace py{

void init_loader(PyObject* module)
{
    // BIN enum
    PyObject* bin_enum = new_enum();
    assign_enum(bin_enum, "ELF32", PyLong_FromLong((int)loader::Format::ELF32), "ELF 32-bits");
    assign_enum(bin_enum, "ELF64", PyLong_FromLong((int)loader::Format::ELF64), "ELF 64-bits");
    // PyDict_SetItemString(bin_enum, "PE32", PyLong_FromLong((int)loader::Format::PE32));
    // PyDict_SetItemString(bin_enum, "PE64", PyLong_FromLong((int)loader::Format::PE64));
    create_enum(module, "BIN", bin_enum, "Supported binary executable formats by the loader");
};

} // namespace py
} // namespace maat
