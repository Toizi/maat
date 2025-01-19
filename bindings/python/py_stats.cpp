#include "python_bindings.hpp"
#include "maat/stats.hpp"

namespace maat{
namespace py{

static int Stats_print(PyObject* self, void * io, int s)
{
    std::cout << MaatStats::instance() << std::flush;
    return 0;
}

static PyObject* Stats_str(PyObject* self)
{
    std::stringstream res;
    res << MaatStats::instance();
    return PyUnicode_FromString(res.str().c_str());
}

static PyObject* Stats_repr(PyObject* self)
{
    return Stats_str(self);
}

PyDoc_STRVAR(Stats_reset_doc,
    "reset()\n"
    "\n"
    "Reset statistics."
);
static PyObject* Stats_reset(PyObject* self)
{
    maat::MaatStats::instance().reset();
    Py_RETURN_NONE;
}

// initialized in init_stats
static Stats_Object* singleton = nullptr;

PyDoc_STRVAR(Stats_instance_doc,
    "instance() -> MaatStats\n"
    "\n"
    "Get the singleton instance."
);
static PyObject* Stats_instance(PyObject* self)
{
    return (PyObject*)singleton;
}

static PyMethodDef Stats_methods[] = {
    {"instance", (PyCFunction)Stats_instance, METH_NOARGS | METH_CLASS, Stats_instance_doc},
    {"reset", (PyCFunction)Stats_reset, METH_NOARGS | METH_CLASS, Stats_reset_doc},
    {NULL, NULL, 0, NULL}
};

// MACROs for generic getter bindings for MaatStats
#define MAAT_DEFINE_STATS_GETTER(property_name) \
static PyObject* Stats_get_##property_name(PyObject* self, void* closure){ \
   return PyLong_FromUnsignedLongLong( \
       MaatStats::instance().property_name() \
   ); \
}

#define MAAT_GETDEF(property_name, docstr) \
{#property_name, Stats_get_##property_name, NULL, docstr, NULL}

MAAT_DEFINE_STATS_GETTER(symptr_read_total_time)
MAAT_DEFINE_STATS_GETTER(symptr_read_average_time)
MAAT_DEFINE_STATS_GETTER(symptr_read_average_range)
MAAT_DEFINE_STATS_GETTER(symptr_read_count)
MAAT_DEFINE_STATS_GETTER(symptr_write_total_time)
MAAT_DEFINE_STATS_GETTER(symptr_write_average_time)
MAAT_DEFINE_STATS_GETTER(symptr_write_average_range)
MAAT_DEFINE_STATS_GETTER(symptr_write_count)
MAAT_DEFINE_STATS_GETTER(executed_insts)
MAAT_DEFINE_STATS_GETTER(executed_ir_insts)
MAAT_DEFINE_STATS_GETTER(lifted_insts)
MAAT_DEFINE_STATS_GETTER(created_exprs)
MAAT_DEFINE_STATS_GETTER(solver_total_time)
MAAT_DEFINE_STATS_GETTER(solver_average_time)
MAAT_DEFINE_STATS_GETTER(solver_calls_count)

static PyGetSetDef Stats_getset[] = {
    MAAT_GETDEF(symptr_read_total_time, "type=int\nTotal time spent solving symbolic pointer reads (in milliseconds)"),
    MAAT_GETDEF(symptr_read_average_time, "type=int\nAverage time spent solving symbolic pointer reads (in milliseconds)"),
    MAAT_GETDEF(symptr_read_average_range, "type=int\nAverage range of symbolic pointer reads"),
    MAAT_GETDEF(symptr_read_count, "type=int\nTotal number of symbolic pointer reads"),
    MAAT_GETDEF(symptr_write_total_time, "type=int\nTotal time spent solving symbolic pointer writes (in milliseconds)"),
    MAAT_GETDEF(symptr_write_average_time, "type=int\nAverage time spent solving symbolic pointer rwrites (in milliseconds)"),
    MAAT_GETDEF(symptr_write_average_range, "type=int\nAverage range of symbolic pointer writes"),
    MAAT_GETDEF(symptr_write_count, "type=int\nTotal number of symbolic pointer writes"),
    MAAT_GETDEF(executed_insts, "type=int\nTotal number of assembly instructions symbolically executed"),
    MAAT_GETDEF(lifted_insts, "type=int\nTotal number of assembly instructions lifted to IR"),
    MAAT_GETDEF(executed_ir_insts, "type=int\nTotal number of IR instructions executed"),
    MAAT_GETDEF(solver_total_time, "type=int\nTotal time spend solving symbolic constraints (in milliseconds)"),
    MAAT_GETDEF(solver_average_time, "type=int\nAverage time spend solving symbolic constraints (in milliseconds)"),
    MAAT_GETDEF(solver_calls_count, "type=int\nTotal number of calls to the solver"),
    {NULL}
};


/* Type description for python stats objects */
PyTypeObject Stats_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "MaatStats",                              /* tp_name */
    sizeof(Stats_Object),                     /* tp_basicsize */
    0,                                        /* tp_itemsize */
    0,                                        /* tp_dealloc */
    (printfunc)Stats_print,                   /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_reserved */
    Stats_repr,                               /* tp_repr */
    0,                                        /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    0,                                        /* tp_hash  */
    0,                                        /* tp_call */
    Stats_str,                                /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                       /* tp_flags */
    "Maat statistics",                        /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    Stats_methods,                            /* tp_methods */
    0,                                        /* tp_members */
    Stats_getset,                             /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    0,                                        /* tp_init */
    0,                                        /* tp_alloc */
    0,                                        /* tp_new */
};

PyObject* get_Stats_Type()
{
    return (PyObject*)&Stats_Type;
}

// Constructor
PyObject* maat_Stats()
{
    // Create object
    PyType_Ready(&Stats_Type);
    Stats_Object* object = PyObject_New(Stats_Object, &Stats_Type);
    return (PyObject*)object;
}

void init_stats(PyObject* module)
{
    register_type(module, (PyTypeObject*)get_Stats_Type());

    // create the singleton
    singleton = PyObject_New(Stats_Object, &Stats_Type);
}

} // namespace py
} // namespace maat
