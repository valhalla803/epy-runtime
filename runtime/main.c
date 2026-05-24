#include <Python.h>

#include <stdio.h>

#include "../include/epy.h"
#include "../include/bundle.h"
#include "../include/cache.h"
#include "../include/antidebug.h"

static PyObject* EPYImport(
    PyObject* self,
    PyObject* args
) {
    const char* module_name;

    if (
        !PyArg_ParseTuple(
            args,
            "s",
            &module_name
        )
    ) {
        return NULL;
    }

    return epy_import_module(module_name);
}

static PyObject* EPYHasModule(
    PyObject* self,
    PyObject* args
) {
    const char* module_name;

    if (
        !PyArg_ParseTuple(
            args,
            "s",
            &module_name
        )
    ) {
        return NULL;
    }

    return PyBool_FromLong(
        epy_bundle_has_module(module_name)
    );
}

static int install_importer() {

    PyObject* sys =
        PyImport_ImportModule("sys");

    if (!sys) {
        return 0;
    }

    PyObject* meta_path =
        PyObject_GetAttrString(
            sys,
            "meta_path"
        );

    Py_DECREF(sys);

    if (!meta_path) {
        return 0;
    }

    PyObject* importlib_abc =
        PyImport_ImportModule(
            "importlib.abc"
        );

    PyObject* importlib_util =
        PyImport_ImportModule(
            "importlib.util"
        );

    if (
        !importlib_abc ||
        !importlib_util
    ) {
        Py_XDECREF(importlib_abc);
        Py_XDECREF(importlib_util);
        Py_DECREF(meta_path);
        return 0;
    }

    PyObject* loader_base =
        PyObject_GetAttrString(
            importlib_abc,
            "Loader"
        );

    PyObject* finder_base =
        PyObject_GetAttrString(
            importlib_abc,
            "MetaPathFinder"
        );

    if (
        !loader_base ||
        !finder_base
    ) {
        Py_XDECREF(loader_base);
        Py_XDECREF(finder_base);
        Py_DECREF(meta_path);
        return 0;
    }

    Py_DECREF(loader_base);
    
    Py_DECREF(finder_base);

    PyRun_SimpleString(
        "import importlib.abc\n"
        "import importlib.util\n"
        "import epy_runtime\n"
        "\n"
        "class EPYLoader(importlib.abc.Loader):\n"
        "\n"
        "    def create_module(self, spec):\n"
        "        return None\n"
        "\n"
        "    def exec_module(self, module):\n"
        "\n"
        "        loaded = epy_runtime.epy_import(module.__name__)\n"
        "\n"
        "        if loaded:\n"
        "            module.__dict__.update(loaded.__dict__)\n"
        "\n"
        "class EPYFinder(importlib.abc.MetaPathFinder):\n"
        "\n"
        "    def find_spec(self, fullname, path=None, target=None):\n"
        "\n"
        "        if not epy_runtime.epy_has_module(fullname):\n"
        "            return None\n"
        "\n"
        "        return importlib.util.spec_from_loader(fullname, EPYLoader())\n"
    );

    PyObject* main_module =
        PyImport_AddModule("__main__");

    PyObject* globals =
        PyModule_GetDict(main_module);

    PyObject* finder_class =
        PyDict_GetItemString(
            globals,
            "EPYFinder"
        );

    if (!finder_class) {
        Py_DECREF(meta_path);
        return 0;
    }

    PyObject* finder_instance =
        PyObject_CallObject(
            finder_class,
            NULL
        );

    if (!finder_instance) {
        Py_DECREF(meta_path);
        return 0;
    }

    PyList_Insert(
        meta_path,
        0,
        finder_instance
    );

    Py_DECREF(finder_instance);

    Py_DECREF(meta_path);

    return 1;
}

static PyObject* EPYInitBundle(
    PyObject* self,
    PyObject* args
) {
    const char* bundle_path;

    if (
        !PyArg_ParseTuple(
            args,
            "s",
            &bundle_path
        )
    ) {
        return NULL;
    }

    epy_protect_process();

    epy_cache_init();

    if (
        !epy_bundle_open(bundle_path)
    ) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "Failed to open bundle"
        );
        return NULL;
    }

    if (!install_importer()) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "Failed to install importer"
        );
        return NULL;
    }

    PyObject* main_module =
        epy_import_module("main");

    if (!main_module) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "Failed to execute main module"
        );
        return NULL;
    }

    Py_DECREF(main_module);

    Py_RETURN_NONE;
}

static PyMethodDef EPYMethods[] = {
    {
        "epy_import",
        EPYImport,
        METH_VARARGS,
        ""
    },
    {
        "epy_has_module",
        EPYHasModule,
        METH_VARARGS,
        ""
    },
    {
        "init_bundle",
        EPYInitBundle,
        METH_VARARGS,
        ""
    },
    {
        NULL,
        NULL,
        0,
        NULL
    }
};

static struct PyModuleDef EPYModule = {
    PyModuleDef_HEAD_INIT,
    "epy_runtime",
    NULL,
    -1,
    EPYMethods
};

PyMODINIT_FUNC PyInit_epy_runtime(void) {
    return PyModule_Create(&EPYModule);
}