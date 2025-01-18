#include "python_bindings.hpp"
#include <iostream>
#include <sstream>
#include <filesystem>

namespace maat{
namespace py{

PyDoc_STRVAR(maat_SimpleStateManager_doc, R"EOF(SimpleStateManager(states_dir: str, base_filename: str = "", delete_on_load: bool = True)

Create a new state manager
:param str states_dir: The directory where to store files containing serialized states
:param str base_filename: (Optional) The base name for serialized state files. The files will be named base_filename_0, base_filename_1, etc
:param bool delete_on_load: (Optional) If set to True, delete serialized state file from the disk when loading the state into the MaatEngine
)EOF");
/* Constructor */
int maat_SimpleStateManager_init(SimpleStateManager_Object* self, PyObject* args, PyObject *kwds)
{
    // Parse args
    std::filesystem::path dir;
    std::string base_filename;
    const char* py_dir = nullptr;
    const char* py_base_filename = nullptr;
    int delete_on_load = 1;

    if( !PyArg_ParseTuple(args, "s|sp", &py_dir, &py_base_filename, &delete_on_load))
    {
        return -1;
    }

    try{
        dir = std::filesystem::path(py_dir);
    } catch(const std::filesystem::filesystem_error& e){
        PyObject *err = PyUnicode_FromFormat("Invalid 'dir' argument: %s", e.what());
        if (err == NULL) {
            return -1;  // Return -1 if there's an error creating the message
        }
        PyErr_SetObject(PyExc_ValueError, err);
        Py_DECREF(err);
        return -1;
    }

    if (py_base_filename != nullptr)
        base_filename = std::string(py_base_filename);
    if (base_filename.empty())
        base_filename = std::string("maat_state");

    self->s = new serial::SimpleStateManager(dir, base_filename, (bool)delete_on_load);
    return 0;
}


// Methods
static void SimpleStateManager_dealloc(PyObject* self){
    delete ((SimpleStateManager_Object*)self)->s; 
    ((SimpleStateManager_Object*)self)->s = nullptr;
    Py_TYPE(self)->tp_free((PyObject *)self);
};

static PyObject* SimpleStateManager_enqueue_state(PyObject* self, PyObject* args)
{
    PyObject* engine;

    if( !PyArg_ParseTuple(args, "O!", get_MaatEngine_Type(), &engine))
    {
        return NULL;
    }
    
    try
    {
        as_simplestate_object(self).s->enqueue_state(*as_engine_object(engine).engine);
    }
    catch(const runtime_exception& e)
    {
        return PyErr_Format(PyExc_RuntimeError, "%s", e.what());
    }

    Py_RETURN_NONE;
}

static PyObject* SimpleStateManager_dequeue_state(PyObject* self, PyObject* args)
{
    PyObject* engine;
    
    if( !PyArg_ParseTuple(args, "O!", get_MaatEngine_Type(), &engine))
    {
        return NULL;
    }

    bool res = false;
    try
    {
        res = as_simplestate_object(self).s->dequeue_state(
            *as_engine_object(engine).engine
        );
    }
    catch(const runtime_exception& e)
    {
        return PyErr_Format(PyExc_RuntimeError, "%s", e.what());
    }

    // IMPORTANT: we need to reinit the object attributes so that they point to the
    // fields of the new engine, otherwise they will continue pointing to the fields
    // of the previous engine
    _clear_MaatEngine_attributes((MaatEngine_Object*)engine); // To decref previous objects
    _init_MaatEngine_attributes((MaatEngine_Object*)engine); // To create new wrapper objects

    if (res)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

static PyMethodDef SimpleStateManager_methods[] = {
    {"enqueue_state", (PyCFunction)SimpleStateManager_enqueue_state, METH_VARARGS, "Save current state of a MaatEngine in pending states list"},
    {"dequeue_state", (PyCFunction)SimpleStateManager_dequeue_state, METH_VARARGS, "Load next pending state into MaatEngine"},
    {NULL, NULL, 0, NULL}
};

/* Type description for python Expr objects */
PyTypeObject SimpleStateManager_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "SimpleStateManager",                     /* tp_name */
    sizeof(SimpleStateManager_Object),        /* tp_basicsize */
    0,                                        /* tp_itemsize */
    (destructor)SimpleStateManager_dealloc,   /* tp_dealloc */
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
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                       /* tp_flags */
    maat_SimpleStateManager_doc,              /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    SimpleStateManager_methods,               /* tp_methods */
    0,                                        /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    (initproc)maat_SimpleStateManager_init,   /* tp_init */
    0,                                        /* tp_alloc */
    PyType_GenericNew,                        /* tp_new */
};

PyObject* get_SimpleStateManager_Type(){
    return (PyObject*)&SimpleStateManager_Type;
};

/* ------------------------------------
 *          Init function
 * ------------------------------------ */
void init_simplestate(PyObject* module)
{
    register_type(module, (PyTypeObject*)get_SimpleStateManager_Type());
}

}
}
