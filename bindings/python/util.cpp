#include "Python.h"
#include <unicodeobject.h>
#include <vector>
#include "maat/value.hpp"
#include "python_bindings.hpp"

namespace maat{
namespace py{

int set_doc(PyObject* obj, const char* docstr){
#ifdef WITH_DOC_STRINGS
    return PyDict_SetItemString(obj, "__doc__", PyUnicode_FromString(docstr));
#else
    return 0;
#endif
}

int set_enum_doc(PyObject* obj, const char *name, const char *docstr) {
    PyObject *enum_docs = PyDict_GetItemString(obj, "_enum_docs");
    assert(enum_docs != nullptr && "_enum_docs not set");
    return PyDict_SetItemString(enum_docs, name, PyUnicode_FromString(docstr));
}

PyObject* new_enum() {
    PyObject* enum_ = PyDict_New();
    PyObject* enum_docs = PyDict_New();
    PyDict_SetItemString(enum_, "_enum_docs", enum_docs);
    return enum_;
}

int assign_enum(PyObject* enum_dict, const char* name, PyObject* value, const char* docstr){
    PyDict_SetItemString(enum_dict, name, value);
    return set_enum_doc(enum_dict, name, docstr);
}

int create_enum(PyObject* module, const char* name, PyObject* enum_dict, const char* docstr){
    set_doc(enum_dict, docstr);
    PyObject* enum_class = create_class(PyUnicode_FromString(name), PyTuple_New(0), enum_dict);
    return PyModule_AddObject(module, name, enum_class);
}

PyObject* create_class(PyObject* name, PyObject* bases, PyObject* dict){
    PyObject* res = PyObject_CallFunctionObjArgs((PyObject*)&PyType_Type, name, bases, dict, NULL);
    Py_CLEAR(name);
    Py_CLEAR(bases);
    Py_CLEAR(dict);
    return res;
}

PyObject* native_to_py(const std::vector<Value>& values)
{
    PyObject* list = PyList_New(0);
    if( list == NULL )
    {
        return PyErr_Format(PyExc_RuntimeError, "%s", "Failed to create new python list");
    }
    for (const Value& e : values)
    {
        if( PyList_Append(list, PyValue_FromValue(e)) == -1)
        {
            return PyErr_Format(PyExc_RuntimeError, "%s", "Failed to add expression to python list");
        }
    }
    return list;
}

PyObject* native_to_py(const std::unordered_set<Constraint>& constraints)
{
    PyObject* list = PyList_New(0);
    if( list == NULL )
    {
        return PyErr_Format(PyExc_RuntimeError, "%s", "Failed to create new python list");
    }
    for (const Constraint& c : constraints)
    {
        if( PyList_Append(list, PyConstraint_FromConstraint(c)) == -1)
        {
            return PyErr_Format(PyExc_RuntimeError, "%s", "Failed to add constraint to python list");
        }
    }
    return list;
}

Number bigint_to_number(size_t bits, PyObject* num)
{
    if (bits <= 64)
    {
        cst_t res = PyLong_AsLongLong(num);
        if (PyErr_Occurred() != nullptr)
        {
            PyErr_Clear();
            res = PyLong_AsUnsignedLongLong(num);
        }
        return Number(bits, res);
    }
    else
    {
        PyObject* str = PyObject_Str(num);
        const char* s = PyUnicode_AsUTF8(str);
        return Number(bits, std::string(s), 10); // base 10
    }
}

PyObject* number_to_bigint(const Number& num)
{
    std::stringstream ss;
    ss << std::hex << num;
    return PyLong_FromString(ss.str().c_str(), NULL, 16);
}

void register_type(PyObject* module, PyTypeObject* type_obj)
{
    // TODO(boyan): We could use PyModule_AddType(module, get_Config_Type()); instead
    // of the cumbersome code below but it's not avaialble before Python 3.10 and we
    // don't want to force Python 3.10 yet
    if (PyType_Ready(type_obj) < 0)
        return;
    Py_INCREF(type_obj);
    if (PyModule_AddObject(module, type_obj->tp_name, (PyObject*)type_obj) < 0) {
        Py_DECREF(type_obj);
    }
}

bool py_to_c_string_set(PySetObject* set, std::set<std::string>& res)
{
    PyObject *iterator = PyObject_GetIter((PyObject*)set);
    PyObject *item;
    bool error = false;
    while ((item = PyIter_Next(iterator))) {
        // Translate item to string
        const char* s = PyUnicode_AsUTF8(item);
        if (s == nullptr)
            error = true;
        else
            res.insert(std::string(s));
        // Release reference when done
        Py_DECREF(item);
        if (error)
            break;
    }
    Py_DECREF(iterator);
    return !error;
}

} // namespace py
} // namespace maat
