#!/usr/bin/env python

"""
setup.py file for GAUSS Engine Python bindings 

For building Python extension _ge.so

"""

import os
import sys
from setuptools import setup, Extension

lib_dir = os.environ.get('MTENGHOME')

if not lib_dir:
    print("Please set your MTENGHOME environment variable. Aborting.")
    sys.exit(1)

is_win = os.name == 'nt'

gauss_module = Extension('_ge', 
      sources=['gauss.i', 'src/gauss.cpp', 'src/gematrix.cpp', 
               'src/gearray.cpp', 'src/gestringarray.cpp', 
               'src/geworkspace.cpp', 'src/workspacemanager.cpp', 
               'src/gesymbol.cpp'], 
      include_dirs=['include', 'src'] + ([lib_dir + '/pthreads'] if is_win else []),
      library_dirs=[lib_dir],
      libraries=['mteng'] + (['pthreadVC2'] if is_win else []),
      define_macros=[('GAUSS_LIBRARY', None)],
      extra_compile_args=['-std=c++11'] if not is_win else None,
      swig_opts=['-c++'] + (['-py3'] if sys.version_info >= (3,0) else []),
      )

setup (name = 'ge',
       version = '0.3',
       author      = "Aptech Systems, Inc.",
       author_email = "matt@aptech.com",
       description = """Python bindings for GAUSS Engine""",
       long_description = """ Python object-oriented front end for the GAUSS Engine """,
       platforms=['Windows', 'Linux'],
       url='http://github.com/aptech/gsoop',
       ext_modules = [gauss_module],
       py_modules = ["ge"],
       license = "MIT",
       )

