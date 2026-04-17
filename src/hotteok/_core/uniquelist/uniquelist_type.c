#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdbool.h>
#include "uniquelist_type.h"


// Pre-declarations
static Py_ssize_t uqls_len(UniqueList *self);


/* **************** */
/* Helper Functions */
/* **************** */

// Helper function to check index validity
// Returns 0 if valid, -1 if invalid (with exception set)
static inline int uqls__check_index_validity(UniqueList *self, Py_ssize_t index) {
    if (index < 0 || index >= uqls_len(self)) {
        PyErr_SetString(PyExc_IndexError, "Index out of range");
        return -1;
    }
    return 0;
}

// Helper function to convert Python index to C index and validate it
// Returns the valid index on success, or -1 on error (with exception set)
static Py_ssize_t uqls__convert_index(UniqueList *self, PyObject *index_obj, bool clamp) {
    if (!PyIndex_Check(index_obj)) {  // __index__
        PyErr_SetString(PyExc_TypeError, "Index must be an integer");
        return -1;
    }

    Py_ssize_t index = PyNumber_AsSsize_t(index_obj, PyExc_IndexError);
    if (index == -1 && PyErr_Occurred()) {
        return -1;
    }

    Py_ssize_t size = uqls_len(self);

    if (index < 0) {
        index += size;  // Set real index
    }

    if (clamp) {
        index = (index < 0) ? 0 :
                (index > size) ? size :
                index;
    } else {
        if (uqls__check_index_validity(self, index) < 0) {
            return -1;
        }
    }

    return index;
}

// Helper function to get item at index, with error handling
// Returns new reference on success, or NULL on error (with exception set)
static inline PyObject *uqls__get_item(UniqueList *self, Py_ssize_t index) {
    PyObject *item = PyList_GET_ITEM(self->list, index);  // Borrowed reference
    Py_INCREF(item);
    return item;
}

// Helper function to write dict[item] = index for a single slot.
// Returns 0 on success, -1 on error (with exception set).
static inline int uqls__dict_set_index(UniqueList *self, PyObject *item, Py_ssize_t index) {
    PyObject *v = PyLong_FromSsize_t(index);
    if (!v) {
        return -1;
    }
    int r = PyDict_SetItem(self->dict, item, v);
    Py_DECREF(v);
    return r;
}

// Refreshes dict[list[j]] = j for j in [start, size).
// Called after positional mutations (insert/pop/delete) to re-align stored indices.
// Returns 0 on success, -1 on error. NOTE: a mid-loop failure leaves the container
// in an inconsistent state — PyDict_SetItem realistically only fails on OOM, and
// the existing code already does not fully roll back on such cascading failures.
static int uqls__reindex_from(UniqueList *self, Py_ssize_t start) {
    Py_ssize_t size = PyList_GET_SIZE(self->list);
    for (Py_ssize_t j = start; j < size; j++) {
        PyObject *k = PyList_GET_ITEM(self->list, j);  // Borrowed
        if (uqls__dict_set_index(self, k, j) < 0) {
            return -1;
        }
    }
    return 0;
}

// Helper function to append an item, with option to ignore existing items
// Returns 1 if appended, 0 if ignored, and -1 on error (with exception set)
static int uqls__append(UniqueList *self, PyObject *item, bool ignore_existing) {
    int contains = PyDict_Contains(self->dict, item);
    if (contains < 0) {
        return -1; // Error occurred (e.g., unhashable item)
    }
    if (contains) {
        if (ignore_existing) {
            return 0; // Ignore and do not add
        }
        PyErr_Format(PyExc_ValueError, "Duplicate item: %R", item);
        return -1;
    }

    Py_ssize_t new_idx = PyList_GET_SIZE(self->list);
    if (PyList_Append(self->list, item) < 0) {
        return -1;
    }
    if (uqls__dict_set_index(self, item, new_idx) < 0) {
        // Rollback list append on dict failure
        PySequence_DelItem(self->list, new_idx);
        return -1;
    }

    return 1; // Added
}

// Helper function to extend from an iterable, with error handling
// Returns 0 on success, -1 on error (with exception set)
static int uqls__extend(UniqueList *self, PyObject *iterable) {
    PyObject *iter = PyObject_GetIter(iterable);
    if (!iter) {
        return -1;
    }

    PyObject *item;
    while ((item = PyIter_Next(iter)) != NULL) {
        int appended = uqls__append(self, item, true);
        Py_DECREF(item);
        if (appended < 0) {
            Py_DECREF(iter);
            return -1;
        }
    }

    Py_DECREF(iter);

    return 0; // Success
}


/* ************* */
/* Magic Methods */
/* ************* */

// .tp_init / __init__
static int uqls_init(UniqueList *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"iterable", NULL};
    PyObject *iterable = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &iterable)) {
        return -1;
    }

    self->dict = PyDict_New();
    if (!self->dict) {
        return -1;
    }

    self->list = PyList_New(0);
    if (!self->list) {
        return -1;
    }

    if (iterable && uqls__extend(self, iterable) < 0) {
        return -1;
    }

    return 0;
}

// DEALLOCATION (Not a magic method, but related to object lifecycle)
static void uqls_dealloc(UniqueList *self) {
    Py_XDECREF(self->list);
    Py_XDECREF(self->dict);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *uqls_copy(UniqueList *self, PyObject *Py_UNUSED(ignored)) {
    UniqueList *copy = (UniqueList *)PyType_GenericNew(&UniqueListType, NULL, NULL);
    if (!copy) {
        return NULL;
    }

    copy->dict = PyDict_Copy(self->dict);
    if (!copy->dict) {
        Py_DECREF(copy);  // dealloc
        return NULL;
    }
    copy->list = PyList_GetSlice(self->list, 0, PyList_GET_SIZE(self->list));
    if (!copy->list) {
        Py_DECREF(copy);  // dealloc
        return NULL;
    }

    return (PyObject *)copy;
}

// .tp_iter / __iter__
static PyObject *uqls_iter(UniqueList *self) {
    return PyObject_GetIter(self->list);
}

// .tp_repr / __repr__
static PyObject *uqls_repr(UniqueList *self) {
    return PyUnicode_FromFormat("UniqueList(%R)", self->list);
}

// .tp_as_sequence->sq_length / __len__
static Py_ssize_t uqls_len(UniqueList *self) {
    return PyList_GET_SIZE(self->list);
}

// .tp_as_sequence->sq_contains / __contains__
static int uqls_contains(UniqueList *self, PyObject *item) {
    return PyDict_Contains(self->dict, item);
}

// .tp_as_sequence->sq_item / __getitem__
static PyObject *uqls_getitem(UniqueList *self, Py_ssize_t index) {
    if (uqls__check_index_validity(self, index) < 0) {
        return NULL;
    }

    return uqls__get_item(self, index);
}

// .tp_as_sequence->sq_ass_item / __setitem__, __delitem__
static int uqls_assitem(UniqueList *self, Py_ssize_t index, PyObject *value) {
    int ret = -1;  // return value

    PyObject *item = uqls_getitem(self, index);
    if (!item) {
        goto end;
    }

    if (PyDict_DelItem(self->dict, item) < 0) {
        goto done;
    }

    if (!value) {
        // Handle deletion: shift tail left, then reindex [index, size)
        if (PySequence_DelItem(self->list, index) < 0) {
            goto rollback;
        }
        if (uqls__reindex_from(self, index) < 0) {
            // Inconsistent state — realistically only on OOM.
            goto end;
        }
    } else {
        // Handle assignment: position unchanged, no reindex needed.
        // Only the old key is removed and the new key mapped to the same index.
        int contains = PyDict_Contains(self->dict, value);
        if (contains < 0) {
            goto rollback;
        }
        if (contains) {
            PyErr_Format(PyExc_ValueError, "Duplicate item: %R", value);
            goto rollback;
        }

        if (uqls__dict_set_index(self, value, index) < 0) {
            goto rollback;
        }
        if (PySequence_SetItem(self->list, index, value) < 0) {
            PyDict_DelItem(self->dict, value); // Rollback dict addition on failure
            goto rollback;
        }
    }

    ret = 0; // Success

    goto done;

// goto
rollback:
    uqls__dict_set_index(self, item, index);
done:
    Py_DECREF(item);
end:
    return ret;
}

// .tp_as_sequence
static PySequenceMethods uqls_as_sequence = {
    .sq_length = (lenfunc)uqls_len,
    .sq_contains = (objobjproc)uqls_contains,
    .sq_item = (ssizeargfunc)uqls_getitem,
    .sq_ass_item = (ssizeobjargproc)uqls_assitem
};


/* ******* */
/* Methods */
/* ******* */

// METH_O
static PyObject *uqls_append(UniqueList *self, PyObject *item) {
    if (uqls__append(self, item, true) < 0) {
        return NULL; // Error occurred (e.g., unhashable item)
    }

    Py_RETURN_NONE;
}

// METH_O
static PyObject *uqls_extend(UniqueList *self, PyObject *iterable) {
    if (uqls__extend(self, iterable) < 0) {
        return NULL; // Error occurred (e.g., unhashable item)
    }

    Py_RETURN_NONE;
}

// METH_FASTCALL
static PyObject *uqls_pop(UniqueList *self, PyObject *const *args, Py_ssize_t nargs) {
    Py_ssize_t size = uqls_len(self);

    if (size == 0) {
        PyErr_SetString(PyExc_IndexError, "pop from empty UniqueList");
        return NULL;
    }

    Py_ssize_t index;
    if (nargs == 0) {
        index = size - 1; // Default to last item
    } else if (nargs == 1) {
        index = uqls__convert_index(self, args[0], false);
        if (index < 0) {
            return NULL; // Invalid index
        }
    } else {
        PyErr_SetString(PyExc_TypeError, "pop() takes at most one argument");
        return NULL;
    }

    PyObject *item = uqls__get_item(self, index);
    if (!item) {
        return NULL; // Failed to get item (unreachable since index is validated)
    }

    if (PyDict_DelItem(self->dict, item) < 0) {
        Py_DECREF(item);
        return NULL;
    }
    if (PySequence_DelItem(self->list, index) < 0) {
        uqls__dict_set_index(self, item, index);  // Best-effort rollback
        Py_DECREF(item);
        return NULL;
    }

    // Reindex tail if anything was shifted. Popping the last element is a no-op.
    if (index < size - 1 && uqls__reindex_from(self, index) < 0) {
        Py_DECREF(item);
        return NULL;
    }

    return item; // Return popped item
}

// METH_O — O(1) via dict lookup
static PyObject *uqls_index(UniqueList *self, PyObject *item) {
    PyObject *v = PyDict_GetItemWithError(self->dict, item);  // Borrowed
    if (!v) {
        if (!PyErr_Occurred()) {
            PyErr_Format(PyExc_ValueError, "%R is not in UniqueList", item);
        }
        return NULL;
    }
    Py_INCREF(v);  // Return new reference to the stored PyLong
    return v;
}

// METH_FASTCALL
static PyObject *uqls_insert(UniqueList *self, PyObject *const *args, Py_ssize_t nargs) {
    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "insert() takes exactly 2 arguments (%zd given)", nargs);
        return NULL;
    }

    Py_ssize_t index = uqls__convert_index(self, args[0], true);
    if (index < 0) {
        return NULL; // Invalid index
    }

    PyObject *item = args[1];

    int contains = PyDict_Contains(self->dict, item);
    if (contains < 0) {
        return NULL; // Error occurred (e.g., unhashable item)
    }
    if (contains) {
        PyErr_Format(PyExc_ValueError, "Duplicate item: %R", item);
        return NULL;
    }

    if (PyList_Insert(self->list, index, item) < 0) {
        return NULL;
    }

    // Reindex [index, size) — covers both the new slot and all shifted entries.
    if (uqls__reindex_from(self, index) < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}

// METH_O
static PyObject *uqls_remove(UniqueList *self, PyObject *item) {
    // O(1) index lookup via dict
    PyObject *v = PyDict_GetItemWithError(self->dict, item);  // Borrowed
    if (!v) {
        if (!PyErr_Occurred()) {
            PyErr_Format(PyExc_ValueError, "UniqueList.remove(x): x not in list");
        }
        return NULL;
    }

    Py_ssize_t index = PyLong_AsSsize_t(v);
    if (index == -1 && PyErr_Occurred()) {
        return NULL;
    }

    if (PyDict_DelItem(self->dict, item) < 0) {
        return NULL;
    }
    if (PySequence_DelItem(self->list, index) < 0) {
        uqls__dict_set_index(self, item, index);  // Best-effort rollback
        return NULL;
    }

    if (uqls__reindex_from(self, index) < 0) {
        return NULL;
    }

    Py_RETURN_NONE; // Success
}

// METH_NOARGS
static PyObject *uqls_tolist(UniqueList *self, PyObject *Py_UNUSED(ignored)) {
    return PyList_GetSlice(self->list, 0, PyList_GET_SIZE(self->list));
}


/* *************** */
/* Type Definition */
/* *************** */

static PyMethodDef uqls_methods[] = {
    {"copy", (PyCFunction)uqls_copy, METH_NOARGS, NULL},
    {"append", (PyCFunction)uqls_append, METH_O, NULL},
    {"extend", (PyCFunction)uqls_extend, METH_O, NULL},
    {"pop", (PyCFunction)uqls_pop, METH_FASTCALL, NULL},
    {"index", (PyCFunction)uqls_index, METH_O, NULL},
    {"insert", (PyCFunction)uqls_insert, METH_FASTCALL, NULL},
    {"remove", (PyCFunction)uqls_remove, METH_O, NULL},
    {"tolist", (PyCFunction)uqls_tolist, METH_NOARGS, NULL},
    {"__class_getitem__", (PyCFunction)Py_GenericAlias, METH_CLASS | METH_O, NULL},
    {NULL, NULL, 0, NULL}  // Sentinel
};

PyTypeObject UniqueListType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "hotteok._core.UniqueList",
    .tp_basicsize = sizeof(UniqueList),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc)uqls_init,
    .tp_dealloc = (destructor)uqls_dealloc,
    .tp_repr = (reprfunc)uqls_repr,
    .tp_iter = (getiterfunc)uqls_iter,
    .tp_as_sequence = &uqls_as_sequence,
    .tp_methods = uqls_methods,
};
