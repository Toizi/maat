#include "Python.h"
#include "python_bindings.hpp"
#include <filesystem>
#include <optional>

namespace maat
{
namespace py
{

PyDoc_STRVAR(
    maat_Cst_doc,
    "Cst(size: int, value: int|str, base: Optional[int]=16) -> Value\n"
    "\n"
    "Create a constant abstract expression.\n"
    "\n"
    ":param int size: Size of the value in bits. Must be greater than 64 if `value` is a `str`\n"
    ":param int|str value: Constant value of the expression\n"
    ":param Optional[int] base: Base of `value` if `value` is a `str`"
);
PyDoc_STRVAR(
    maat_Var_doc,
    "Var(size: int, name: str) -> Value\n"
    "\n"
    "Create an abstract variable.\n"
    "\n"
    ":param int size: Size of the expression in bits\n"
    ":param str name: Unique name identifying the variable"
);
PyDoc_STRVAR(
    maat_Concat_doc,
    "Concat(upper: Value, lower: Value) -> Value\n"
    "\n"
    "Concatenate two abstract expressions"
);
PyDoc_STRVAR(
    maat_Extract_doc,
    "Extract(val: Value, higher: int, lower: int) -> Value\n"
    "\n"
    "Bitfield extract from an abstract expression"
);
PyDoc_STRVAR(
    maat_Sext_doc,
    "Sext(new_size: int, val: Value) -> Value\n"
    "\n"
    "Sign-extend an abstract value"
);
PyDoc_STRVAR(
    maat_Zext_doc,
    "Zext(new_size: int, val: Value) -> Value\n"
    "\n"
    "Zero-extend an abstract value"
);
PyDoc_STRVAR(
    maat_ULE_doc,
    "ULE(left: int|Value, right: int|Value) -> Constraint\n"
    "\n"
    "Create an unsigned less-equal constraint. At least one of left or right must be a `Value`."
);
PyDoc_STRVAR(
    maat_ULT_doc,
    "ULT(left: int|Value, right: int|Value) -> Constraint\n"
    "\n"
    "Create an unsigned less-than constraint. At least one of left or right must be a `Value`."
);
PyDoc_STRVAR(
    maat_ITE_doc,
    "ITE(constraint: Constraint, if_true: Value|int, if_false: Value|int) -> Constraint\n"
    "\n"
    "Create an If-Then-Else expression from a Constraint and two abstract expressions"
);
// Module methods
PyMethodDef module_methods[] = {
    // Expressions
    {"Cst", (PyCFunction)maat_Cst, METH_VARARGS | METH_KEYWORDS, maat_Cst_doc},
    {"Var", (PyCFunction)maat_Var, METH_VARARGS | METH_KEYWORDS, maat_Var_doc},
    {"Concat", (PyCFunction)maat_Concat, METH_VARARGS, maat_Concat_doc},
    {"Extract", (PyCFunction)maat_Extract, METH_VARARGS, maat_Extract_doc},
    {"Sext", (PyCFunction)maat_Sext, METH_VARARGS, maat_Sext_doc},
    {"Zext", (PyCFunction)maat_Zext, METH_VARARGS, maat_Zext_doc},
    {"ULE", (PyCFunction)maat_ULE, METH_VARARGS, maat_ULE_doc},
    {"ULT", (PyCFunction)maat_ULT, METH_VARARGS, maat_ULT_doc},
    {"ITE", (PyCFunction)maat_ITE, METH_VARARGS, maat_ITE_doc},
    // EVM
    {"EVMTransaction", (PyCFunction)maat_Transaction, METH_VARARGS, "Create an ethereum transaction"},
    {"contract", (PyCFunction)maat_contract, METH_VARARGS, "Get EVM contract associated with a MaatEngine"},
    {"new_evm_runtime", (PyCFunction)maat_new_evm_runtime, METH_VARARGS, "Create new EVM contract runtime for `new_engine` based on runtime for `old_engine`"},
    {"increment_block_number", (PyCFunction)maat_increment_block_number, METH_VARARGS, "Increment the current block number by an abstract value"},
    {"increment_block_timestamp", (PyCFunction)maat_increment_block_timestamp, METH_VARARGS, "Increment the current block timestamp by an abstract value"},
    {"set_evm_bytecode", (PyCFunction)maat_set_evm_bytecode, METH_VARARGS, "Set runtime bytecode for the contract associated to an engine"},
    {"allow_symbolic_keccak", (PyCFunction)maat_allow_symbolic_keccak, METH_VARARGS, "Enable/disable symbolic KECCAK hashes"},
    {"evm_get_static_flag", (PyCFunction)maat_evm_get_static_flag, METH_VARARGS, "Get EVM static flag"},
    {"evm_set_static_flag", (PyCFunction)maat_evm_set_static_flag, METH_VARARGS, "Set EVM static flag"},
    {"evm_set_gas_price", (PyCFunction)maat_evm_set_gas_price, METH_VARARGS, "Set EVM gas price"},
    {NULL}
};

// Module information
PyModuleDef maat_module_def = {
    PyModuleDef_HEAD_INIT,
    "maat",
    nullptr,
    -1,      // m_size
    module_methods, // m_methods
    nullptr, // m_slots
    nullptr, // m_traverse
    nullptr, // m_clear
    nullptr  // m_free
};

std::optional<std::filesystem::path> get_maat_module_directory()
{
    // Add a lookup directory for sleigh files based on the module location
    PyObject* maat_module = PyState_FindModule(&maat_module_def);
    if (not maat_module)
        return std::nullopt;
    PyObject* filename_obj = PyModule_GetFilenameObject(maat_module);
    if (not filename_obj)
        return std::nullopt;
    const char* filename = PyUnicode_AsUTF8(filename_obj);
    if (not filename)
        return std::nullopt;
    return std::filesystem::path(filename).parent_path();
}

} // namespace py
} // namespace maat

using namespace maat;
using namespace maat::py;
PyMODINIT_FUNC PyInit_maat()
{
    Py_Initialize();
    PyObject* module = PyModule_Create(&maat_module_def);

    init_arch(module);
    init_expression(module);
    init_constraint(module);
    init_cpu(module);
    init_regs(module);
    init_memory(module);
    init_engine(module);
    init_event(module);
    init_path(module);
    init_loader(module);
    init_env(module);
    init_config(module);
    init_stats(module);
    init_evm(module);
    init_settings(module);
    init_process(module);
    init_solver(module);
    init_simplestate(module);
    init_info(module);
    init_filesystem(module);

    PyState_AddModule(module, &maat_module_def);

    return module;
}
