#include "python_bindings.hpp"
#include <boolobject.h>

namespace maat{
namespace py{
    
static void EventManager_dealloc(PyObject* self){
    if( ! as_event_object(self).is_ref){
        delete as_event_object(self).manager;
    }
    as_event_object(self).manager = nullptr;
    Py_TYPE(self)->tp_free((PyObject *)self);
};

static int EventManager_print(PyObject* self, void * io, int s)
{
    std::cout << *(as_event_object(self).manager);
    return 0;
}

static PyObject* EventManager_str(PyObject* self)
{
    std::stringstream res;
    res << *(as_event_object(self).manager);
    return PyUnicode_FromString(res.str().c_str());
}

static PyObject* EventManager_repr(PyObject* self)
{
    return EventManager_str(self);
}

PyDoc_STRVAR(
    EventManager_add_doc,
    "add(event: EVENT, when: WHEN, name: str='', filter: int|Tuple[int, int]=-1, callbacks: List[Callable]=[], group: str='') -> int\n"
    "\n"
    ":param EVENT event: Event to hook\n"
    ":param WHEN when: When to trigger the hook (before or after the event)\n"
    ":param str name: (Optional) Unique name to identify the hook\n"
    ":param int|Tuple[int,int] filter: (Optional) Address range on which to trigger the hook. It is used only for memory access events and the EVENT.EXEC event."
    "The parameter can be a single integer to monitor a single address, or a tuple of two integers to monitor a range of addresses.\n"
    ":param List[Callable] callbacks: (Optional) List of callbacks to be called every time the hook is triggered\n"
    ":param str group: (Optional) Group of the hook"
);
static PyObject* EventManager_add(PyObject* self, PyObject*args, PyObject* keywords)
{
    int int_event, int_when;
    const char* name = "";
    const char* group = "";
    PyObject* filter = NULL;
    const char * reg_name = nullptr;
    unsigned long long  filter_min = 0,
                        filter_max = 0xffffffffffffffff;
    PyObject* callbacks = NULL;
    PyObject* callback_data = NULL;
    std::vector<event::EventCallback> callbacks_list;

    char* keywd[] = {"", "", "name", "filter", "callbacks", "data", "group", NULL};

    if( !PyArg_ParseTupleAndKeywords(
        args, keywords, "ii|s(KK)OOs", keywd, &int_event, &int_when, &name, &filter_min, &filter_max, &callbacks, &callback_data, &group))
    {
        PyErr_Clear();
        if( !PyArg_ParseTupleAndKeywords(
        args, keywords, "ii|sOOOs", keywd, &int_event, &int_when, &name, &filter, &callbacks, &callback_data, &group))
        {
            return NULL;
        }
    }

    // Check callbacks list
    if (callbacks != NULL)
    {
        // Check if it's a list
        if (not PyList_Check(callbacks))
        {
            return PyErr_Format(PyExc_TypeError, "'callbacks' parameter must be a list of callbacks");
        }
        for (int i = 0; i < PyList_Size(callbacks); i++)
        {
            PyObject* cb = PyList_GetItem(callbacks, i);
            if (not PyCallable_Check(cb))
            {
                return PyErr_Format(PyExc_TypeError, "Callback number %d is not a callable object", i);
            }
            callbacks_list.push_back(event::EventCallback(cb, callback_data));
        }
    }

    event::Event event = (event::Event)int_event;
    event::When when = (event::When)int_when;
    event::AddrFilter addr_filter;
    // Get filter
    if (filter == NULL)
    {
        // If not default, then set it to specified value
        if (filter_min != 0 or filter_max != 0xffffffffffffffff)
        {
            addr_filter = event::AddrFilter(filter_min, filter_max);
        }
        // Otherwise let the default filter
    }
    else
    {
        if (not PyLong_Check(filter))
            return PyErr_Format(PyExc_TypeError, "Expected integer or integer pair for 'filter' argument");
        addr_filter = event::AddrFilter(PyLong_AsUnsignedLongLong(filter));
    }

    // Add hook
    try
    { 
        as_event_object(self).manager->add(
            event, when, callbacks_list, std::string(name), addr_filter, std::string(group)
        );
    }
    catch(const event_exception& e)
    {
        return PyErr_Format(PyExc_ValueError, "%s", e.what());
    }

    Py_RETURN_NONE;
};

PyDoc_STRVAR(
    EventManager_disable_doc,
    "disable(hook_name: str)\n"
    "\n"
    "Disable the hook named *hook_name*"
);
static PyObject* EventManager_disable(PyObject* self, PyObject *args)
{
    const char* name;
    
    if( !PyArg_ParseTuple(args, "s", &name) ){
        return NULL;
    }
    try
    {
        as_event_object(self).manager->disable(std::string(name));
    }
    catch (const event_exception& e)
    {
        return PyErr_Format(PyExc_ValueError, "%s", e.what());
    }
    Py_RETURN_NONE;
};

PyDoc_STRVAR(
    EventManager_disable_group_doc,
    "disable_group(group: str)\n"
    "\n"
    "Disable the hook group registered as *group*"
);
static PyObject* EventManager_disable_group(PyObject* self, PyObject *args)
{
    const char* name;
    
    if( !PyArg_ParseTuple(args, "s", &name) ){
        return NULL;
    }
    try
    {
        as_event_object(self).manager->disable_group(std::string(name));
    }
    catch (const event_exception& e)
    {
        return PyErr_Format(PyExc_ValueError, "%s", e.what());
    }
    Py_RETURN_NONE;
};

PyDoc_STRVAR(
    EventManager_enable_doc,
    "enable(hook_name: str)\n"
    "\n"
    "Enable the hook named *hook_name*"
);
static PyObject* EventManager_enable(PyObject* self, PyObject *args)
{
    const char* name;
    
    if( !PyArg_ParseTuple(args, "s", &name) ){
        return NULL;
    }

    try
    {
        as_event_object(self).manager->enable(std::string(name));
    }
    catch (const event_exception& e)
    {
        return PyErr_Format(PyExc_ValueError, "%s", e.what());
    }
    Py_RETURN_NONE;
};

PyDoc_STRVAR(
    EventManager_enable_group_doc,
    "enable_group(group: str)\n"
    "\n"
    "Enable the hook group registered as *group*"
);
static PyObject* EventManager_enable_group(PyObject* self, PyObject *args)
{
    const char* name;
    
    if( !PyArg_ParseTuple(args, "s", &name) ){
        return NULL;
    }

    try
    {
        as_event_object(self).manager->enable_group(std::string(name));
    }
    catch (const event_exception& e)
    {
        return PyErr_Format(PyExc_ValueError, "%s", e.what());
    }
    Py_RETURN_NONE;
};

PyDoc_STRVAR(
    EventManager_disable_all_doc,
    "disable_all()\n"
    "\n"
    "Disable all hooks"
);
static PyObject* EventManager_disable_all(PyObject* self )
{
    as_event_object(self).manager->disable_all();
    Py_RETURN_NONE;
};


static PyMethodDef EventManager_methods[] = {
    {"add", (PyCFunction)EventManager_add, METH_VARARGS | METH_KEYWORDS, EventManager_add_doc},
    {"disable", (PyCFunction)EventManager_disable, METH_VARARGS, EventManager_disable_doc},
    {"disable_group", (PyCFunction)EventManager_disable_group, METH_VARARGS, EventManager_disable_group_doc},
    {"disable_all", (PyCFunction)EventManager_disable_all, METH_NOARGS, EventManager_disable_all_doc},
    {"enable", (PyCFunction)EventManager_enable, METH_VARARGS, EventManager_enable_doc},
    {"enable_group", (PyCFunction)EventManager_enable_group, METH_VARARGS, EventManager_enable_group_doc},
    {NULL, NULL, 0, NULL}
};

static PyMemberDef EventManager_members[] = {
    {NULL}
};

/* Type description for python BreakopintManager objects */
static PyTypeObject EventManager_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "EventManager",                             /* tp_name */
    sizeof(EventManager_Object),                /* tp_basicsize */
    0,                                        /* tp_itemsize */
    (destructor)EventManager_dealloc,           /* tp_dealloc */
    (printfunc)EventManager_print,               /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_reserved */
    EventManager_repr,                                        /* tp_repr */
    0,                                        /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    0,                                        /* tp_hash  */
    0,                                        /* tp_call */
    EventManager_str,                                        /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                       /* tp_flags */
    "Event hooks manager",                  /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    EventManager_methods,                /* tp_methods */
    EventManager_members,                /* tp_members */
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


PyObject* PyEventManager_FromEventManager(event::EventManager* m, bool is_ref)
{
    EventManager_Object* object;
    
    // Create object
    PyType_Ready(&EventManager_Type);
    object = PyObject_New(EventManager_Object, &EventManager_Type);
    if (object != nullptr)
    {
        object->manager = m;
        object->is_ref = is_ref;
    }
    return (PyObject*)object;
}

// Init enums
void init_event(PyObject* module)
{
    // EVENT enum
    PyObject* event_enum = new_enum();
    assign_enum(event_enum, "EXEC", PyLong_FromLong((int)event::Event::EXEC),
                "An instruction in a given address range is executed");
    assign_enum(event_enum, "BRANCH", PyLong_FromLong((int)event::Event::BRANCH),
                "A branch operation (conditional or absolute) is executed");
    assign_enum(event_enum, "MEM_R", PyLong_FromLong((int)event::Event::MEM_R),
                "A given address range is read");
    assign_enum(event_enum, "MEM_W", PyLong_FromLong((int)event::Event::MEM_W),
                "A given address range is written");
    assign_enum(event_enum, "MEM_RW", PyLong_FromLong((int)event::Event::MEM_RW),
                "A combination of MEM_R | MEM_RW");
    assign_enum(event_enum, "PATH", PyLong_FromLong((int)event::Event::PATH),
                "A path constraint (conditional branch with symbolic/concolic condition) is encountered");
    assign_enum(event_enum, "REG_R", PyLong_FromLong((int)event::Event::REG_R),
                "A given register is read");
    assign_enum(event_enum, "REG_W", PyLong_FromLong((int)event::Event::REG_W),
                "A given register is written");
    assign_enum(event_enum, "REG_RW", PyLong_FromLong((int)event::Event::REG_RW),
                "A combination of REG_R | REG_W");
    create_enum(module, "EVENT", event_enum, "Events on which a breakpoint can be triggered");

    // Action enum
    PyObject* action_enum = new_enum();
    assign_enum(action_enum, "CONTINUE", PyLong_FromLong((int)event::Action::CONTINUE),
                "Contine execution");
    assign_enum(action_enum, "HALT", PyLong_FromLong((int)event::Action::HALT),
                "Stop execution");
    assign_enum(action_enum, "ERROR", PyLong_FromLong((int)event::Action::ERROR),
                "An error occurred in the callback");
    create_enum(module, "ACTION", action_enum,
                "Action returned by hook callbacks for the execution engine");

    // WHEN enum
    PyObject* when_enum = new_enum();
    assign_enum(when_enum, "BEFORE", PyLong_FromLong((int)event::When::BEFORE),
                "Trigger callbacks BEFORE the associated event");
    assign_enum(when_enum, "AFTER", PyLong_FromLong((int)event::When::AFTER),
                "Trigger callbacks AFTER the associated event");
    create_enum(module, "WHEN", when_enum,
                "An enum indicating when callbacks must be triggered");

    register_type(module, &EventManager_Type);
}

} // namespace py
} // namespace maat
