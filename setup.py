#!/usr/bin/env python

"""
setup.py file for GAUSS Engine Python bindings

For building Python extension _ge.so

"""

import os
import sys
from setuptools import setup, Extension

lib_dir = os.environ.get("MTENGHOME")

if not lib_dir:
    print("Please set your MTENGHOME environment variable. Aborting.")
    sys.exit(1)

is_win = os.name == "nt"

version = "0.3.2"
author = "Aptech Systems, Inc."
author_email = "matt@aptech.com"
description = """Python bindings for GAUSS Engine"""
long_description = """ Python object-oriented front end for the GAUSS Engine """
platforms = ["Windows", "Linux"]
url = "http://github.com/aptech/gsoop"
license = "MIT"

sources = ["src/gauss.cpp", "src/gematrix.cpp",
         "src/gearray.cpp", "src/gestringarray.cpp",
         "src/geworkspace.cpp", "src/workspacemanager.cpp",
         "src/gesymbol.cpp"]
include_dirs = ["include", "src"] + ([lib_dir + "/pthreads"] if is_win else [])
library_dirs = [lib_dir]
define_macros = [("GAUSS_LIBRARY", None)]
extra_compile_args = ["-std=c++11"] if not is_win else None
swig_opts = ["-c++"] + (["-py3"] if sys.version_info >= (3,0) else [])

def mk_extension(ge_suffix=''):
    return Extension("_ge{}".format(ge_suffix),
          sources = sources + ['ge{}.i'.format(ge_suffix)],
          include_dirs = include_dirs,
          library_dirs = [lib_dir],
          libraries = ["mteng{}".format(ge_suffix)] + (["pthreadVC2"] if is_win else []),
          define_macros = define_macros,
          extra_compile_args = extra_compile_args,
          swig_opts = swig_opts
    )


ge_module = mk_extension()
gert_module = mk_extension('rt')

setup (name = "ge",
       version = version,
       author = author,
       author_email = author_email,
       description = description,
       long_description = long_description,
       platforms = platforms,
       url = url,
       license = license,
       ext_modules = [ge_module, gert_module],
       py_modules = ["ge", "gert"],
)

