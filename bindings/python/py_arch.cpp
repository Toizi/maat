#include "python_bindings.hpp"
#include "maat/arch.hpp"

namespace maat{
namespace py{

void init_arch(PyObject* module)
{
    // ARCH enum
    PyObject* arch_enum = new_enum();
    assign_enum(arch_enum, "X86", PyLong_FromLong((int)Arch::Type::X86), "Intel X86 (32 bits)");
    assign_enum(arch_enum, "X64", PyLong_FromLong((int)Arch::Type::X64), "Intel X86-64 (64 bits)");
    assign_enum(arch_enum, "EVM", PyLong_FromLong((int)Arch::Type::EVM), "Ethereum EVM");
    create_enum(module, "ARCH", arch_enum, "Architectures supported for emulation");
};

} // namespace py
} // namespace maat
