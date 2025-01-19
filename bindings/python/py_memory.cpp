#include "python_bindings.hpp"

namespace maat{
namespace py{
    
// ============= MemEngine ===============

static void MemEngine_dealloc(PyObject* self){
    if( ! as_mem_object(self).is_ref){
        delete ((MemEngine_Object*)self)->mem;
    }
    as_mem_object(self).mem = nullptr;
    Py_TYPE(self)->tp_free((PyObject *)self);
};

static PyObject* MemEngine_str(PyObject* self){
    std::stringstream res;
    res << *((MemEngine_Object*) self)->mem;
    return PyUnicode_FromString(res.str().c_str());
}

static int MemEngine_print(PyObject* self, void * io, int s){
    std::cout << *((MemEngine_Object*) self)->mem << std::flush;
    return 0;
}

static PyObject* MemEngine_repr(PyObject* self) {
    return MemEngine_str(self);
}

PyDoc_STRVAR(
    MemEngine_map_doc,
    "map(start: int, end: int, flags: PERM=PERM.RWX, name: str='')\n"
    "\n"
    "Map a memory region. The requested region to map is force-aligned to the memory defaut page size (0x1000)."
    "For instance map(0xfff, 0x1001, ...) will actually map addresses from 0x0 up to 0x1fff.\n"
    ":param int start: Start address of the map (included)\n"
    ":param int end: End address of the map (included)\n"
    ":param PERM flags: (Optional) Memory permissions of the map\n"
    ":param str name: (Optional) Name of the map\n"
);
static PyObject* MemEngine_map(PyObject* self, PyObject* args, PyObject* keywords) {
    unsigned long long start, end;
    unsigned short flags = maat::mem_flag_rwx;
    char* name = NULL;
    std::string name_str;

    char* keywds[] = {"", "", "flags", "name", NULL};
    
    if( !PyArg_ParseTupleAndKeywords(args, keywords, "KK|Hs", keywds, &start, &end, &flags, &name)){
        return NULL;
    }

    if( name != NULL){
        name_str = std::string(name);
    }

    try{
        as_mem_object(self).mem->map(start, end, flags, name_str);
    }catch(mem_exception e){
        return PyErr_Format(PyExc_RuntimeError, "%s", e.what());
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(
    MemEngine_read_doc,
    "read(addr: int|Value, size: int) -> Value\n"
    "\n"
    "Read a value from memory.\n"
    "\n"
    ":param int|Value addr: Address to read. If the address is not concrete, the method performs a _symbolic pointer read_\n"
    ":param int size: Number of bytes to read\n"
);
static PyObject* MemEngine_read(PyObject* self, PyObject* args) {
    unsigned int nb_bytes;
    Value res;
    PyObject* addr = nullptr;
    
    if(PyArg_ParseTuple(args, "OI", &addr, &nb_bytes)){
        if( PyObject_TypeCheck(addr, (PyTypeObject*)get_Value_Type()) ){
            try{
                // Handles both symbolic and concrete addresses
                res = as_mem_object(self).mem->read(*(as_value_object(addr).value), nb_bytes);
            }catch(const mem_exception& e){
                return PyErr_Format(PyExc_RuntimeError, "%s", e.what());
            }
        }else if(PyLong_Check(addr)){
            try{
                as_mem_object(self).mem->read(res, PyLong_AsUnsignedLongLong(addr), nb_bytes);
            }catch(const mem_exception& e){
                return PyErr_Format(PyExc_RuntimeError, "%s", e.what());
            }
        }else{
            return PyErr_Format(PyExc_TypeError, "%s", "read(): first argument must be int or Expr");
        }
    }else{
        return NULL;
    }
    return PyValue_FromValue(res);
}

PyDoc_STRVAR(
    MemEngine_read_buffer_doc,
    "read_buffer(addr: int|Value, nb_elems: int, elem_size: int=1) -> List[Value]\n"
    "\n"
    "Read a buffer from memory and return it as a list of expressions.\n"
    "\n"
    ":param int|Value addr: Address to read. Can be an abstract address. Does not work with fully symbolic addresses.\n"
    ":param int nb_elems: Number of elements to read\n"
    ":param int elem_size: (Optional) Size of each element\n"
);
static PyObject* MemEngine_read_buffer(PyObject* self, PyObject* args) {
    PyObject* addr;
    unsigned int nb_elems, elem_size=1;
    std::vector<Value> res;
    PyObject* list;

    if( !PyArg_ParseTuple(args, "OI|I", &addr, &nb_elems, &elem_size)){
        return NULL;
    }

    if( PyObject_TypeCheck(addr, (PyTypeObject*)get_Value_Type()) ){
        try{
            res = as_mem_object(self).mem->read_buffer(*(as_value_object(addr).value), nb_elems, elem_size);
        }catch(mem_exception e){
            return PyErr_Format(PyExc_RuntimeError, "%s", e.what());
        }
    }else if(PyLong_Check(addr)){
        try{
            res = as_mem_object(self).mem->read_buffer(PyLong_AsUnsignedLongLong(addr), nb_elems, elem_size);
        }catch(mem_exception e){
            return PyErr_Format(PyExc_RuntimeError, "%s", e.what());
        }
    }else{
        return PyErr_Format(PyExc_TypeError, "%s", "read_buffer(): first argument must be int or Expr");
    }

    // Translate expressions list into python list
    list = PyList_New(0);
    if( list == NULL ){
        return PyErr_Format(PyExc_RuntimeError, "%s", "Failed to create new python list");
    }
    for (const Value& val : res)
    {
        if( PyList_Append(list, PyValue_FromValue(val)) == -1){
            return PyErr_Format(PyExc_RuntimeError, "%s", "Failed to add expression to python list");
        }
    }
    return list;
}

PyDoc_STRVAR(
    MemEngine_read_str_doc,
    "read_str(addr: int|Value, length: int=0) -> bytes\n"
    "\n"
    "Read a string from memory.\n"
    "\n"
    ":param int|Value addr: Address to read. Can be an abstract address. Does not work with fully symbolic addresses.\n"
    ":param int length: (Optional) Length of the string to read. If `0`, reads a null terminated string"
);
static PyObject* MemEngine_read_str(PyObject* self, PyObject* args) {
    PyObject* addr;
    unsigned int len=0;
    std::string res;
    PyObject* bytes;

    if( !PyArg_ParseTuple(args, "O|I", &addr, &len)){
        return NULL;
    }

    if( PyObject_TypeCheck(addr, (PyTypeObject*)get_Value_Type()) ){
        try{
            res = as_mem_object(self).mem->read_string(*(as_value_object(addr).value), len );
        }catch(const mem_exception& e){
            return PyErr_Format(PyExc_RuntimeError, "%s", e.what());
        }
    }else if(PyLong_Check(addr)){
        try{
            res = as_mem_object(self).mem->read_string(PyLong_AsUnsignedLongLong(addr), len);
        }catch(const mem_exception& e){
            return PyErr_Format(PyExc_RuntimeError, "%s", e.what());
        }
    }else{
        return PyErr_Format(PyExc_TypeError, "%s", "read_string(): first argument must be int or Expr");
    }

    // Translate string into python bytes
    bytes = PyBytes_FromStringAndSize(res.c_str(), res.size());
    if( bytes == NULL ){
        return PyErr_Format(PyExc_RuntimeError, "%s", "Failed to translate string to python bytes");
    }

    return bytes;
}


PyDoc_STRVAR(
    MemEngine_write_doc,
    "write(addr: int|Value, value: int, size: int, ignore_flags: bool=False)\n"
    "\n"
    "Write a concrete value to memory\n"
    "\n"
    ":param int|Value addr: Address to read. Can be an abstract address. Does not work with fully symbolic addresses.\n"
    ":param int value: Value to write\n"
    ":param int size: Size in bytes of the value to write\n"
    ":param bool ignore_flags: (Optional) If `True`, writes without checking `PERM.W` access flag"
);
// TODO: split these into several write_* functions since these overloads are impossible to have in python
static PyObject* MemEngine_write(PyObject* self, PyObject* args, PyObject* keywords)
{
    addr_t concrete_addr;
    PyObject* addr = nullptr;
    Value val_addr;
    Expr e = nullptr;
    char * data = nullptr;
    Py_ssize_t data_len;
    PyObject* arg2 = nullptr;
    PyObject* arg3 = nullptr;
    int ignore_flags = 0; // Default False 

    char * keywds[] = {"", "", "", "ignore_flags", NULL};

    if( !PyArg_ParseTupleAndKeywords(args, keywords, "OO|Op", keywds, &addr, &arg2, &arg3, &ignore_flags)){
        return NULL;
    }

    // Check addr first
    if( PyLong_Check(addr)){
        concrete_addr = PyLong_AsUnsignedLongLong(addr);
    }else if( PyObject_TypeCheck(addr, (PyTypeObject*)get_Value_Type())){
        val_addr = *(as_value_object(addr).value);
    }else{
        return PyErr_Format(PyExc_TypeError, "MemEngine.write(): address must be 'int' or 'Expr'"); 
    }

    try{
        // Check arguments types, function is overloaded
        // (addr, expr)
        if( PyObject_TypeCheck(arg2, (PyTypeObject*)get_Value_Type()) ){
            if (not val_addr.is_none())
                as_mem_object(self).mem->write(val_addr, *(as_value_object(arg2).value), (bool)ignore_flags);
            else
                as_mem_object(self).mem->write(concrete_addr, *(as_value_object(arg2).value), nullptr, false, (bool)ignore_flags);
        // (addr, cst, nb_bytes)
        }else if(arg3 != nullptr && PyLong_Check(arg2) && PyLong_Check(arg3)){
            if (not val_addr.is_none())
                as_mem_object(self).mem->write(
                    val_addr,
                    PyLong_AsLongLong(arg2),
                    PyLong_AsUnsignedLong(arg3),
                    (bool)ignore_flags
                );
            else
                as_mem_object(self).mem->write(
                    concrete_addr,
                    PyLong_AsLongLong(arg2),
                    PyLong_AsUnsignedLong(arg3),
                    (bool)ignore_flags
                );
        // (addr, buffer, nb_bytes)
        }else if( PyBytes_Check(arg2) ){
            PyBytes_AsStringAndSize(arg2, &data, &data_len);
            if( arg3 != nullptr){
                if( !PyLong_Check(arg3)){
                    return PyErr_Format(PyExc_TypeError, "MemEngine.write(): 3rd argument must be int");
                }
                // Optional length argument, parse it
                if(PyLong_AsSsize_t(arg3) < data_len){
                    data_len = PyLong_AsSsize_t(arg3);
                }
            }
            if (not val_addr.is_none())
                as_mem_object(self).mem->write_buffer(
                    val_addr,
                    (uint8_t*)data,
                    (unsigned int)data_len,
                    (bool)ignore_flags
                );
            else
                as_mem_object(self).mem->write_buffer(
                    concrete_addr,
                    (uint8_t*)data,
                    (unsigned int)data_len,
                    (bool)ignore_flags
                );
        }else{
            return PyErr_Format(PyExc_TypeError, "MemEngine.write(): got wrong types for arguments");
        }
    }catch(mem_exception e){
        return PyErr_Format(PyExc_RuntimeError, "%s", e.what());
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(
    MemEngine_make_concolic_doc,
    "make_concolic(addr: int, nb_elems: int, elem_size: int, name: str) -> str\n"
    "\n"
    "Make memory content concolic. The method creates a new set of nb_elems abstract variables of size elem_size bytes. "
    "It follows the same naming strategy as `MemEngine.make_symbolic()` for the variables. The current concrete values "
    "present in memory are automatically bound to the new variables in the engine's `VarContext` (so the created variables "
    "are not purely symbolic and can still be concretized). The method returns the base name of the created symbolic variables\n"
    "\n"
    ":param int addr: Start address\n"
    ":param int nb_elems: Number of elements to create\n"
    ":param int elem_size: Size of each element in bytes\n"
    ":param str name: Preferred base name for the created variables"
);
PyObject* MemEngine_make_concolic(PyObject* self, PyObject* args){
    unsigned long long addr;
    unsigned int nb_elems, elem_size;
    char * name = "";
    std::string res_name;

    if( ! PyArg_ParseTuple(args, "KIIs", &addr, &nb_elems, &elem_size, &name)){
        return NULL;
    }

    try{
        res_name = as_mem_object(self).mem->make_concolic(addr, nb_elems, elem_size, std::string(name));
    }catch(mem_exception e){
        return PyErr_Format(PyExc_RuntimeError, "%s", e.what());
    }catch(var_context_exception e){
        return PyErr_Format(PyExc_RuntimeError, "%s", e.what());
    }

    return PyUnicode_FromString(res_name.c_str());
}


PyDoc_STRVAR(
    MemEngine_make_symbolic_doc,
    "make_symbolic(addr: int, nb_elems: int, elem_size: int, name: str) -> str\n"
    "\n"
    "Make memory content purely symbolic. The method creates a new set of nb_elems purely symbolic variables of "
    "size `elem_size` bytes. The variables are named according to the name parameter. If name='myvar' then the "
    "created variables are named 'myvar_0', 'myvar_1', etc. If the requested name isn't available, another name "
    "will be automatically selected (for example 'myvar1' instead of 'myvar', then the created variables are "
    "'myvar1_0', 'myvar1_1', etc). In any case, the method returns the chosen base name of the created symbolic "
    "variables.\n"
    "\n"
    ":param int addr: Start address\n"
    ":param int nb_elems: Number of elements to create\n"
    ":param int elem_size: Size of each element in bytes\n"
    ":param str name: Preferred base name for the created variables"
);
PyObject* MemEngine_make_symbolic(PyObject* self, PyObject* args){
    unsigned long long addr;
    unsigned int nb_elems, elem_size;
    char * name = "";
    std::string res_name;
    
    if( ! PyArg_ParseTuple(args, "KIIs", &addr, &nb_elems, &elem_size, &name)){
        return NULL;
    }

    try{
        res_name = as_mem_object(self).mem->make_symbolic(addr, nb_elems, elem_size, std::string(name));
    }catch(mem_exception e){
        return PyErr_Format(PyExc_RuntimeError, "%s", e.what());
    }catch(var_context_exception e){
        return PyErr_Format(PyExc_RuntimeError, "%s", e.what());
    }

    return PyUnicode_FromString(res_name.c_str());
}


static PyMethodDef MemEngine_methods[] = {
    {"map", (PyCFunction)MemEngine_map, METH_VARARGS | METH_KEYWORDS, MemEngine_map_doc},
    {"read", (PyCFunction)MemEngine_read, METH_VARARGS, MemEngine_read_doc},
    {"read_buffer", (PyCFunction)MemEngine_read_buffer, METH_VARARGS, MemEngine_read_buffer_doc},
    {"read_str", (PyCFunction)MemEngine_read_str, METH_VARARGS, MemEngine_read_str_doc},
    {"write", (PyCFunction)MemEngine_write, METH_VARARGS | METH_KEYWORDS, MemEngine_write_doc},
    {"make_concolic", (PyCFunction)MemEngine_make_concolic, METH_VARARGS, MemEngine_make_concolic_doc},
    {"make_symbolic", (PyCFunction)MemEngine_make_symbolic, METH_VARARGS, MemEngine_make_symbolic_doc},
    {NULL, NULL, 0, NULL}
};

static PyMemberDef MemEngine_members[] = {
    {NULL}
};

/* Type description for python MemEngine objects */
static PyTypeObject MemEngine_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "MemEngine",                             /* tp_name */
    sizeof(MemEngine_Object),                /* tp_basicsize */
    0,                                        /* tp_itemsize */
    (destructor)MemEngine_dealloc,           /* tp_dealloc */
    (printfunc)MemEngine_print,              /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_reserved */
    MemEngine_repr,                           /* tp_repr */
    0,                                        /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    0,                                        /* tp_hash  */
    0,                                        /* tp_call */
    MemEngine_str,                            /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                       /* tp_flags */
    "Memory engine",                          /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    MemEngine_methods,                       /* tp_methods */
    MemEngine_members,                       /* tp_members */
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

/* Constructors */
PyObject* PyMemEngine_FromMemEngine(MemEngine* mem, bool is_ref)
{
    MemEngine_Object* object;

    // Create object
    PyType_Ready(&MemEngine_Type);
    object = PyObject_New(MemEngine_Object, &MemEngine_Type);
    if( object != nullptr ){
        object->mem = mem;
        object->is_ref = is_ref;
    }
    return (PyObject*)object;
}

void init_memory(PyObject* module)
{
    /* PERM enum */
    PyObject* mem_enum = new_enum();
    assign_enum(mem_enum, "R", PyLong_FromLong(maat::mem_flag_r), "");
    assign_enum(mem_enum, "W", PyLong_FromLong(maat::mem_flag_w), "");
    assign_enum(mem_enum, "X", PyLong_FromLong(maat::mem_flag_x), "");
    assign_enum(mem_enum, "RW", PyLong_FromLong(maat::mem_flag_rw), "");
    assign_enum(mem_enum, "RX", PyLong_FromLong(maat::mem_flag_rx), "");
    assign_enum(mem_enum, "WX", PyLong_FromLong(maat::mem_flag_wx), "");
    assign_enum(mem_enum, "RWX", PyLong_FromLong(maat::mem_flag_rwx), "");
    create_enum(module, "PERM", mem_enum, "Memory access permissions");

    register_type(module, &MemEngine_Type);
};

} // namespace py
} // namespace maat
