#!/usr/bin/env python

"""
setup.py file for GAUSS Engine Python bindings 

For building Python extension _ge.so

"""

import os, sys
from setuptools import setup, Extension

lib_dir = os.environ.get('MTENGHOME')

if not lib_dir:
    print("Please set your MTENGHOME environment variable. Aborting.")
    sys.exit(1)

is_win = os.name == 'nt'
os.environ["CC"] = "g++"
os.environ["CXX"] = "g++"

gauss_module = Extension('_ge', 
      sources=['gauss.i', 'src/gauss.cpp', 'src/gematrix.cpp', 
               'src/gearray.cpp', 'src/gestringarray.cpp', 
               'src/geworkspace.cpp', 'src/workspacemanager.cpp', 
               'src/gesymbol.cpp', 'src/gesymtype.cpp'], 
      include_dirs=['include', 'src'] + ([lib_dir + '/pthreads'] if is_win else []),
      library_dirs=[lib_dir],
      libraries=['mteng'] + (['pthreadVC2'] if is_win == 'nt' else []),
      define_macros=[('GAUSS_LIBRARY', None)],
      swig_opts=['-c++', '-py3'],
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
       py_modules = ["gauss"],
       license = "MIT",
       )

