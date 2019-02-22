gsoop 0.3.2
=====

A light-weight OOP wrapper for various target languages that allows developers to interface with the GAUSS Engine.

We use the [SWIG](http://www.swig.org) library to generate language-specific extensions from C++ for us.

The following languages are supported with more planned:

+ Python
+ PHP
+ C#

While building from source allows complete control over
the compile process, we have elected to also release binaries for popular platforms.

You can:
+ Create and destroy workspaces, using multiple for threaded situations
+ Compile, save, load and execute programs.
+ Manipulate data in the symbol table
+ Set up callbacks for integration with GAUSS's input/output routines

[TOC]

## Configuration

The API will look for the `MTENGHOME` environment variable.
This will be the location the engine was extracted to (i.e. `C:\mteng` on Windows, `$HOME/mteng` on Linux)

Variable          | Value
:-----------------|:-----------
`MTENGHOME`     | `/home/user/mteng`

The following environment variables must **contain** the specified values

### Windows
Variable   | Value
:----------|:-----------
`PATH`     | Append `C:\mteng`

The presence of this value allows the engine DLL to be found and loaded properly.

Ensure you do not __replace__ `PATH` with the directory.

### Linux

Variable          | Value
:-----------------|:-----------
`LD_LIBRARY_PATH` | Append `/home/user/mteng`

Note: Please adjust the paths accordingly to your specific installation.

### SWIG
If you are building from source, you will need to have the SWIG library available:

http://www.swig.org/download.html

Once installed, please ensure `swig` is available in your terminal by placing its installation directory in your `PATH` environment variable, as building requires it.

__EXCEPTION__: If you are not *modifying* any source files, the auto generated files have been included in the respective `python` and `php` sub-directories. Please refer to php5/php7 based on the version of PHP being used.

## Installation
Please refer to the vendor documentation for language-level installation.

### Python
The Python installation uses the `distribute` package.

### Binary

#### Windows

Select the correct `egg` file according to your architecture.

Please ensure your Python installation contains the `setuptools` package, which provides the `easy_install` application.

Instructions and files can be found at: https://pypi.python.org/pypi/setuptools#windows

    $ easy_install ge-0.3.2-py3.6-win-amd64.egg

#### Linux 

    $ easy_install ge-0.3.2-py3.6-linux-x86_64.egg

### Source

If using Windows, please ensure the following are correctly in your `PATH`
environment variable

    C:\Python36\;C:\Python36\Scripts\

Note: Tested with Python 3.6 (MSVC 2015)

Just like the traditional `make && make install`, compiling a Python extension requires 2 steps:

```bash
python setup.py build_ext -i
python setup.py install
```

#### Linux or Cygwin

```bash
# The setup.py utilizes the 'MTENGHOME' environment variable.
# Please ensure this is set appropriately to your GAUSS Engine installation directory.
tar -xvf ge-0.3.2.tar.gz
cd ge-0.3.2
python setup.py build_ext -i      # First build the extension and create the ge.py file
python setup.py install           # Everything compiled, now install
  
# ROOT ONLY
# If installation must be done as root, force set the environment variable
tar -xvf ge-0.3.2.tar.gz
cd ge-0.3.2
export MTENGHOME=/home/user/mteng
# If you need customized wrapper~
python setup.py build_ext -i # First build the extension and create the ge.py file
python setup.py install      # Everything compiled, now install
```

#### Windows

Due to `mteng.dll` being compiled with Visual Studio 2015, it is necessary at this time to use MSVC to compile the Python extension.

All commands are run through the appropriate Visual Studio command prompt.

32-bit: `Start -> Microsoft Visual Studio 2015 -> Visual Studio Tools -> Visual Studio 2015 Command Prompt`

64-bit: `Start -> Microsoft Visual Studio 2015 -> Visual Studio Tools -> Visual Studio 2015 x64 Win64 Command Prompt`

If your user lacks permissions to access the Python
installation directory, you may have to run `cmd` as Administrator

Execute the following via a `cmd` terminal:

```bash
# The setup.py utilizes the 'MTENGHOME' environment variable.
# Please ensure this is set appropriately to your GAUSS Engine installation directory or set it on the command line.
# unzip ge-0.3.2.zip to a directory of your choice
cd ge-0.3.2
set MTENGHOME=C:\mteng
python setup.py build_ext -i      # First build the extension and create the ge.py file
python setup.py install           # Everything compiled, now install
```

If you omit the `build_ext -i` step, the `ge.py` generated file will __NOT__ be placed in Python package directory.

The result of this behavior is the following when attempting to `import ge`:

```py
    >>> import ge
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    ImportError: No module named ge
```

### PHP (Linux Only)

#### Source

[CMake](http://www.cmake.org) is our build tool of choice for building the PHP extension.

Currently, only Linux is supported/tested. The CMake file uses information from the `php-config` binary to configure itself appropriately.

The following commands are executed from the root of the source path:

    $ mkdir build
    $ cd build
    # You could also use cmake-gui to create the Makefile

To use the pre-generated wrappers, configure as follows:

    $ cmake -DMTENGHOME=/home/user/mteng -G"Unix Makefiles" ..

If you have modified source files and need to regenerate the wrappers:

    $ cmake -DMTENGHOME=/home/user/mteng -G"Unix Makefiles" -DSWIG=ON ..

Now build the extension

    $ make

You should now have a `ge.so` file, as well as `ge.php`, in your current directory.

This will need to be used with any project that makes use of the GAUSS Engine, as the first line of any program will be

    include("ge.php");

#### Binary

Place the `ge.so` file in your PHP installation's extension directory. If a valid directory was found during the CMake configuration phase, you can run `make installext` to automatically copy the newly built library into the appropriate extension directory.

If you prefer to do this manually, the location can be found by either:

1. `php-config --extension-dir`
2. Looking for the `extension_dir` value in your `php.ini` file.

For use with a web server, the extension must be loaded directly by PHP, requiring the following change to your `php.ini`:

    ; load gauss engine extension
    extension=ge.so

If running PHP from the command line, changes should be instantaneous.

Please refresh services that utilize PHP to pick up on the new extension.

## Development

This section introduces an implementation of the GAUSS Engine.

### Hello World!

Getting started with the GAUSS Engine is extremely simple.

This assumes the library has been made available to the target language properly.

The following snippet will be described step-by-step in detail:

#### Python

```py
from ge import GAUSS, IGEProgramOutput

class Output(IGEProgramOutput):
    def invoke(self, message):
        print message,

ge = GAUSS()

out = Output()
ge.setProgramOutputAll(out)

if not ge.initialize():
    print "Initialization failed."
    sys.exit(1)     # import sys for usage

ge.executeString("print \"Hello World!\"")
ge.shutdown()
```

#### PHP

```php
include("ge.php");

class Output extends IGEProgramOutput {
    function invoke($message) {
        echo $message;
    }
}

$ge = new GAUSS();

$out = new Output();

$ge->setProgramOutputAll($out);

if (!$ge->initialize()) {
    echo "Initialization failed.";
    return;
}

$ge->executeString("print \"Hello World!\"");
$ge->shutdown();
```

### Step 1: Import the required library
Before we can make use of the functionality, we have to make the engine library available to us. This is quite simple and straight-forward.

#### Python

```py
from ge import GAUSS, IGEProgramOutput
```

#### PHP

```php
include("ge.php");
```

If you compiled from source and you're not sure where `ge.php` is, it will be in the `build` directory.

### Step 2: Instantiate the GAUSS Engine object

After importing the correct library, we need to instantiate our engine object.
There are multiple constructors available that suit a variety of scenarios.

In this example, we will be using the default constructor.
This method relies on an existing environment variable in an effort to locate the GAUSS home directory.

By default, this environment variable is `MTENGHOME`.

#### Python

```py
ge = GAUSS()  # Look up value for MTENGHOME
```

#### PHP

```php
$ge = new GAUSS();  // Look up value for MTENGHOME
```

__Hint:__ Check out the `GAUSS(string)` constructor, as it allows you to pass in a custom environment variable to use.

### Step 3: Set up callbacks

Technically, this step _can_ be skipped, but creating appropriate callbacks allows you control over specific aspects of program functionality.

While some of these must be implemented to work correctly (i.e. setting a string input callback), others have default implementations that don't
necessarily require a callback (i.e. program output routing to `stdout`), it is generally a good idea to do so.

The following 3 steps are core to setting up a callback:
1. Define the callback by deriving one of the available callback classes
2. Set the `thisown` property to __0__ (release ownership) if the object would go out of scope before the `GAUSS` object
3. Assign the callback

Pretty straight-forward!

We first define a callback class, inheriting the appropriate base class; in this case IGEProgramOutput.

Simply override the callback method `invoke` and print out the `message` argument.

__Important Note:__

We must set the `thisown` property of the newly instantiated object to `0` if it will go out of scope before the `GAUSS` object

This is necessary because ownership of this object will be transferred to the GAUSS object.

Without setting this, if the `out` var was to go out of scope, it could be deleted and GAUSS will now attempt to reference a non-existent callback.

#### Python

```py
# Step 1
class Output(IGEProgramOutput):
    def invoke(self, message):
        print message,

# Step 2
out = Output().__disown__() # Prevent garbage collection

# Step 3
ge.setProgramOutputAll(out)
```

#### PHP

```php
// Step 1
class Output extends IGEProgramOutput {
    function invoke($message) {
        echo $message;
    }
}

$out = new Output();

// Step 2
$out->thisown = 0; // Prevent garbage collection

// Step 3
$ge->setProgramOutputAll($out);
```

_Note:_ We used the GAUSS.setProgramOutputAll method, which is used to set _both_ the program and error output.

If you wish to separate the callbacks for different functionality, you can use the GAUSS.setProgramOutput
and GAUSS.setProgramErrorOutput methods.

### Step 4: Initialize the GAUSS Engine

Now that you've made it to this step, if all has been configured correctly, initializing the Engine is quite easy.

#### Python

```py
if not ge.initialize():
    print "Initialization failed."
    sys.exit(1)
```

#### PHP

```php
if (!$ge->initialize()) {
    echo "Initialization failed.";
    return;
}
```

The `initialize` method returns a true/false value depending on whether it was successful or not.

### Step 5: Home Free
With everything set up, we are free to run whatever code we choose!

For this small example, we just want the engine to tell us "Hello World!" through the callback we've assigned.

#### Python

```py
ge.executeString("print \"Hello World!\"")
```

#### PHP

```php
$ge->executeString("print \"Hello World!\"");
```

will result in the output:

```
Hello World!
```

### Step 6: Shutdown the GAUSS Engine
Shutting down the GAUSS Engine is as easy as telling it to do so:

#### Python

```py
ge.shutdown()
```

#### PHP

```php
$ge->shutdown();
```

This will destroy any active workspaces and perform any cleanup that might be necessary.

## Conclusion
This wraps up the basics of using the GAUSS Engine API. The documentation is full of code snippets that should help you
easily interface with the Engine for your chosen task.

A template file has also been included which represents a full setup
and tear-down with space in the middle to drop in code snippets.
This file will be aptly named in relation to the language it applies to:

Language | Filename
:--------|:--------
Python   | `python/template_example.py`
PHP      | `php/template_example.php`

### Unit Tests
Some simple unit tests have also been provided. The PHP unit test example uses [PHP Unit](https://phpunit.de) and can ran accordingly:

    php -f phpunit.phar unit_test.php

The CMake configure stage will also attempt to locate a PHPUnit binary, and if found, will offer a 'unittest' make target that can be used:

    make unittest

