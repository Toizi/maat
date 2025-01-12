#include "python_bindings.hpp"

namespace maat{
namespace py{
    
// ==================== CPU ====================

static void CPU_dealloc(PyObject* self){
    delete as_cpu_object(self).varctx; as_cpu_object(self).varctx = nullptr;
    if( not as_cpu_object(self).is_ref)
    {
        delete ((CPU_Object*)self)->cpu;
    }
    as_cpu_object(self).cpu = nullptr;
    Py_TYPE(self)->tp_free((PyObject *)self);
};

static PyObject* CPU_str(PyObject* self){
    std::stringstream res;
    as_cpu_object(self).cpu->ctx().print(res, *as_cpu_object(self).arch);
    return PyUnicode_FromString(res.str().c_str());
}

static int CPU_print(PyObject* self, void * io, int s){
    as_cpu_object(self).cpu->ctx().print(std::cout, *as_cpu_object(self).arch);
    return 0;
}

static PyObject* CPU_repr(PyObject* self) {
    return CPU_str(self);
}

static PyMethodDef CPU_methods[] = {
    {NULL, NULL, 0, NULL}
};

static PyMemberDef CPU_members[] = {
    {"regs", T_OBJECT_EX, offsetof(CPU_Object, regs), READONLY, "Symbolic Variables Context"},
    {NULL}
};

/* Type description for python CPU objects */
static PyTypeObject CPU_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "CPU",                                    /* tp_name */
    sizeof(CPU_Object),                       /* tp_basicsize */
    0,                                        /* tp_itemsize */
    (destructor)CPU_dealloc,                  /* tp_dealloc */
    (printfunc)CPU_print,                     /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_reserved */
    CPU_repr,                                 /* tp_repr */
    0,                                        /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    0,                                        /* tp_hash  */
    0,                                        /* tp_call */
    CPU_str,                                  /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                       /* tp_flags */
    "Emulated CPU",                           /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    CPU_methods,                              /* tp_methods */
    CPU_members,                              /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    0,                                        /* tp_init */
    0,                                        /* tp_alloc */
    0,                                        /* tp_new */
};
PyObject* get_CPU_Type(){
    return (PyObject*)&CPU_Type;
}

PyObject* PyCPU_FromCPUAndArchAndVarContext(ir::CPU* cpu, bool is_ref, Arch* arch, std::shared_ptr<VarContext>& ctx)
{
    CPU_Object* object;

    // Create object
    PyType_Ready(&CPU_Type);
    object = PyObject_New(CPU_Object, &CPU_Type);
    if( object != nullptr ){
        object->cpu = cpu;
        object->is_ref = is_ref;
        object->arch = arch;
        object->varctx = new std::shared_ptr<VarContext>(ctx);
        object->regs = PyRegs_FromCPU((PyObject*)object);
    }
    return (PyObject*)object;
}

// Init
void init_cpu(PyObject* module)
{
    register_type(module, (PyTypeObject*)get_CPU_Type());
}

} // namespace py
} // namespace maat
