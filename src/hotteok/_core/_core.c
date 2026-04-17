#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "uniquelist/uniquelist_type.h"


static struct PyModuleDef module_def = {
    PyModuleDef_HEAD_INIT,
    .m_name = "hotteok._core",
    .m_doc = "Hotteok core module",
    .m_size = -1,
};

PyMODINIT_FUNC PyInit__core(void) {
    PyObject *m = PyModule_Create(&module_def);
    if (!m) {
        return NULL;
    }

    if (PyModule_AddType(m, &UniqueListType) < 0) {
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
