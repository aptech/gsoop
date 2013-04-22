#!/usr/bin/env python

"""
setup.py file for GAUSS Engine Python bindings 

For building Python extension _gauss.so

"""

import os, sys
from setuptools import setup, Extension

lib_dir = os.environ.get('MTENGHOME13')

if lib_dir is None:
    print "Please set your MTENGHOME13 environment variable. Aborting."
    sys.exit(1)

os.environ["CC"] = "g++"
os.environ["CXX"] = "g++"

gauss_module = Extension('_gauss', 
      sources=['gauss.i', 'src/gauss.cpp', 'src/gematrix.cpp', 'src/gearray.cpp', 'src/gestring.cpp', 'src/gestringarray.cpp', 'src/geworkspace.cpp', 'src/workspacemanager.cpp', 'src/gesymbol.cpp', 'src/gesymtype.cpp'], 
      include_dirs=['include', 'src', lib_dir + '/pthreads'],
      library_dirs=[lib_dir],
      libraries=['mteng'],
      swig_opts=['-c++'],
      )

setup (name = 'gauss',
       version = '0.1',
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

