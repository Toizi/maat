#include "python_bindings.hpp"

namespace maat{
namespace py{

int Regs_set_attro_impl(PyObject *self, PyObject *attr, PyObject *value)
{
    Regs_Object &regs = as_regs_object(self);
    CPU_Object &cpu_obj = as_cpu_object(regs.cpu);

    std::string name(PyUnicode_AsUTF8(attr));
    try
    {
        // Get reg number
        ir::reg_t reg = cpu_obj.arch->reg_num(name);
        // Check if value to set is expression or integer
        if (PyObject_TypeCheck(value, (PyTypeObject*)get_Value_Type()))
        {
            cpu_obj.cpu->ctx().set(reg, *(as_value_object(value).value));
        }
        else if (PyLong_Check(value))
        {
            int overflow = 0;
            cst_t int_val = PyLong_AsLongLongAndOverflow(value, &overflow);
            if (overflow == 0)
                cpu_obj.cpu->ctx().set(reg, int_val);
            else // More than 64 bits, set as number
            {
                Number number(cpu_obj.arch->reg_size(reg));
                PyObject* repr = PyObject_Repr(value);
                std::string s = std::string(PyUnicode_AsUTF8(repr));
                number.set_mpz(s, 10); // Base 10 because python repr() uses base 10
                cpu_obj.cpu->ctx().set(reg, number);
            }
        }
        else
        {
            PyErr_SetString(PyExc_RuntimeError, "Invalid value: expected 'int' or 'Expr'");
            return 1;
        }
    }
    catch(const ir_exception& e)
    {
        std::stringstream ss; 
        ss << "No register named " << name;
        PyErr_SetString(PyExc_AttributeError, ss.str().c_str());
        return 1;
    }
    catch(const generic_exception& e)
    {
        std::stringstream ss; 
        ss << "Error setting attribute " << name << ": " << e.what();
        PyErr_SetString(PyExc_AttributeError, ss.str().c_str());
        return 1;
    }
    catch(const std::exception& e)
    {
        PyErr_SetString(PyExc_AttributeError, e.what());
        return 1;
    }

    return 0;
}

// wrapper around PyObject_GenericGetAttr to make the regular attributes also available
// first tries to retrieve a register by name and if that fails, falls back to the generic impl
int Regs_set_attro(PyObject *self, PyObject *attr_name, PyObject *v) {
    int32_t rc = -1;
    PyObject * type, * value, * traceback;

    // First try to set a dynamic attribute.
    rc = Regs_set_attro_impl(self, attr_name, v);

    // If that was unsuccessful, set an attribute in our .tp_dict
    if (rc != 0) {
        PyErr_Fetch(&type, &value, &traceback);
        rc = PyObject_GenericSetAttr(self, attr_name, v);

        // Use the original error, if necessary
        if (rc != 0) {
            PyErr_Restore(type, value, traceback);
        } else {
            Py_XDECREF(type);
            Py_XDECREF(value);
            Py_XDECREF(traceback);
        }
    }

    return rc;
}

PyObject* Regs_get_attro_impl(PyObject *self, PyObject *attr)
{
    Regs_Object &regs = as_regs_object(self);
    CPU_Object &cpu_obj = as_cpu_object(regs.cpu);
    std::string name(PyUnicode_AsUTF8(attr));
    try
    {
        ir::reg_t reg = cpu_obj.arch->reg_num(name);
        return PyValue_FromValueAndVarContext(cpu_obj.cpu->ctx().get(reg), *(cpu_obj.varctx));
    }
    catch(const ir_exception& e)
    {
        return PyErr_Format(PyExc_AttributeError, "No register named %s", attr);
    }
    catch(const std::exception& e)
    {
        return PyErr_Format(PyExc_AttributeError, "Error getting attribute %s: %s", attr, e.what());
    }
}

// wrapper around PyObject_GenericGetAttr to make the regular attributes also available
// first tries to retrieve a register by name and if that fails, falls back to the generic impl
PyObject* Regs_get_attro(PyObject *self, PyObject *attr_name) {
    PyObject *res, *type, *value, *traceback;

    // First try to get a dynamic attribute
    res = Regs_get_attro_impl(self, attr_name);

    // If that was unsuccessful, get an attribute out of our .tp_dict
    if (!res) {
        PyErr_Fetch(&type, &value, &traceback);
        res = PyObject_GenericGetAttr(self, attr_name);

        // Use the original error, if necessary
        if (!res) {
            PyErr_Restore(type, value, traceback);
        } else {
            Py_XDECREF(type);
            Py_XDECREF(value);
            Py_XDECREF(traceback);
        }
    }

    return res;
}


/* Type description for python Regs objects */
static PyTypeObject Regs_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "Regs",                                   /* tp_name */
    sizeof(Regs_Object),                      /* tp_basicsize */
    0,                                        /* tp_itemsize */
    0,                                        /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_reserved */
    0,                                        /* tp_repr */
    0,                                        /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    0,                                        /* tp_hash  */
    0,                                        /* tp_call */
    0,                                        /* tp_str */
    (getattrofunc)Regs_get_attro,             /* tp_getattro */
    (setattrofunc)Regs_set_attro,             /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                       /* tp_flags */
    "CPU registers",                          /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    0,                                        /* tp_methods */
    0,                                        /* tp_members */
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
PyObject* get_Regs_Type(){
    return (PyObject*)&Regs_Type;
}

/* Constructors */
PyObject* PyRegs_FromCPU(PyObject* cpu){
    Regs_Object* object;

    // Create object
    PyType_Ready(&Regs_Type);
    object = PyObject_New(Regs_Object, &Regs_Type);
    if( object != nullptr ){
        object->cpu = cpu;
    }
    return (PyObject*)object;
}

// Init 
void init_regs(PyObject* module)
{
    register_type(module, (PyTypeObject*)get_Regs_Type());
}

} // namespace py
} // namespace maat
