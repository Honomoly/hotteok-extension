#ifndef HOTTEOK_UNIQUELIST_TYPE_H
#define HOTTEOK_UNIQUELIST_TYPE_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>


extern PyTypeObject UniqueListType;


typedef struct {
    PyObject_HEAD
    PyObject *dict;   // item -> PyLong(index)
    PyObject *list;
} UniqueList;


#endif // HOTTEOK_UNIQUELIST_TYPE_H
