gSoup 0.1
=====

A set of target language bindings that allows developers to interface with the GAUSS Engine.

We use the [SWIG](http://www.swig.org) library to generate language-specific extensions from C++ for us.

Two languages are supported with more planned:

+ Python
+ PHP

While building from source allows complete control over
the compile process, we have elected to also release binaries for popular platforms.

You can:
+ Create and destroy workspaces, using multiple for threaded situations
+ Compile, save, load and execute programs.
+ Manipulate data in the symbol table
+ Set up callbacks for integration with GAUSS's input/output routines

## Configuration ##

The API will look for the `MTENGHOME13` environment variable.
This will be the location the engine was extracted to (i.e. `C:\mteng13` on Windows, `/home/user/mteng13` on Linux)

Variable          | Value
:-----------------|:-----------
`MTENGHOME13`     | `/home/user/mteng13`

The following environment variables must **contain** the specified values

### Windows ###
Variable   | Value
:----------|:-----------
`PATH`     | Append `C:\mteng13`

The presence of this value allows the engine DLL to be found and loaded properly.

Ensure you do not __replace__ `PATH` with the directory.

### Linux and Unix ###

Variable          | Value
:-----------------|:-----------
`LD_LIBRARY_PATH` | Append `/home/user/mteng13`
`LD_PRELOAD`      | Append `/lib/x86_64-linux-gnu/libpthread.so.0:/home/user/mteng13/bin/libiomp5.so`

Note: Please adjust the paths accordingly to your specific installation. In this case, the `libpthread.so.0` reference is for Ubuntu 64-bit.

### SWIG ###
If you are building from source, you will need to have the SWIG library available:

http://www.swig.org/download.html

Once installed, please ensure `swig` is available in your terminal by placing its installation directory in your `PATH` environment variable, as building requires it.

__EXCEPTION__: If you are not *modifying* any source files, the auto generated files have been included in the respective `python` and `php` sub-directories.

You can place these in `src` and skip the steps involving SWIG.

## Installation ##
Please refer to the vendor documentation for language-level installation.

### Python ###
The Python installation uses the `distribute` package.

### Binary ###

#### Windows ####

Select the correct `egg` file according to your architecture.

Please ensure your Python installation contains the `setuptools` package, which provides the `easy_install` application.

Instructions and files can be found at: https://pypi.python.org/pypi/setuptools#windows

    $ easy_install gauss-0.1-py2.7-win-amd64.egg

#### Linux ####

    $ easy_install gauss-0.1-py2.7-linux-x86_64.egg

### Source ###

If using Windows, please ensure the following are correctly in your `PATH`
environment variable

    C:\Python27\;C:\Python27\Scripts\

Note: Tested with Python 2.7.4

Compilation of a Python extension requires 2 steps:

Makefile syntax  | Python syntax
:----------------|:-----------
`make`           | `python setup.py build_ext -i`
`make install`   | `python setup.py install`

#### Linux or Cygwin ####

~~~{.bash}
# The setup.py utilizes the 'MTENGHOME13' environment variable.
# Please ensure this is set appropriately to your GAUSS Engine installation directory.
tar -xvf gauss-0.1.tar.gz
cd gauss-0.1
python setup.py build_ext -i      # First build the extension and create the gauss.py file
python setup.py install           # Everything compiled, now install

# ROOT ONLY
# If installation must be done as root, force set the environment variable
tar -xvf gauss-0.1.tar.gz
cd gauss-0.1
sudo MTENGHOME13=/home/user/mteng python setup.py build_ext -i      # First build the extension and create the gauss.py file
sudo MTENGHOME13=/home/user/mteng python setup.py install           # Everything compiled, now install
~~~

#### Windows ####

Due to `mteng.dll` being compiled with VS 2008, it is necessary at this time to use MSVC to compile the Python extension.

All commands are run through the appropriate Visual Studio command prompt.

32-bit: `Start -> Microsoft Visual Studio 2008 -> Visual Studio Tools -> Visual Studio 2008 Command Prompt`

64-bit: `Start -> Microsoft Visual Studio 2008 -> Visual Studio Tools -> Visual Studio 2008 x64 Win64 Command Prompt`

If your user does have appropriate permissions to access the Python
installation directory, you may have to run `cmd` as Administrator

Execute the following via a `cmd` terminal:

~~~{.bash}
# The setup.py utilizes the 'MTENGHOME13' environment variable.
# Please ensure this is set appropriately to your GAUSS Engine installation directory.
# unzip gauss-01.zip to a directory of your choice
cd gauss-0.1
python setup.py build_ext -i      # First build the extension and create the gauss.py file
python setup.py install           # Everything compiled, now install
~~~

If you omit the `build_ext -i` step, the `gauss.py` generated file will __NOT__ be placed in Python package directory.

The result of this behavior is the following when attempting to `import gauss`:

    >>> import gauss
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    ImportError: No module named gauss

### PHP ###

#### Source ####

[CMake](http://www.cmake.org) is our build tool of choice. Get it.

There are a couple steps common between both platforms that should be done ahead of time.

1. Set PHP specific environment variables.

    Our CMake file uses the `PHP_INCLUDE` environment variable to locate PHP headers.
    It expects a semi-colon delimited list of paths.

    For convenience, the Linux implementation does a string replace of '`-I`' with a semi-colon, so the following will work fine.

        # Linux
        $ export PHP_INCLUDE=`php-config --includes`

        # Windows you have to create it yourself. Newlines have been removed to clearly show the paths
        # The actual command should have them all appended to each other separated by a semi-colon
        set PHP_INCLUDE=C:\development\php5.4ts\dev;
                        C:\development\php-sdk\php54dev\vc9\x86\php-5.4.14-src;
                        C:\development\php-sdk\php54dev\vc9\x86\php-5.4.14-src\main;
                        C:\development\php-sdk\php54dev\vc9\x86\php-5.4.14-src\Zend;
                        C:\development\php-sdk\php54dev\vc9\x86\php-5.4.14-src\TSRM;
                        C:\development\php-sdk\php54dev\vc9\x86\php-5.4.14-src\ext

    __Note__ on Windows, the environment variable contains `php5.4ts\dev` as part of the include path.

    This ensures `php5ts.lib` will be located properly at link-time.
2. Execute SWIG manually (If you are using pre-generated files from the appropriate language's directory, you can skip this step)

    Unlike our Python installation script, CMake does not automatically execute SWIG on the source files.

    Run one directory up from `src`:

        swig -c++ -php -outdir src -o src/gauss_wrap.cpp gauss.i

After executing `swig`, you will find a newly created `gauss.php` in the `src` directory.

This will need to be used with any project that makes use of the GAUSS Engine, as the first line of any program will be

    include("gauss.php");

#### Linux ####

The following commands are executed from the root of the source path

    $ mkdir build
    $ cd build
    # You could also use cmake-gui to create the Makefile
    $ ccmake ..
    $ make

You should now have a `gauss.so` file in your current  directory.

Refer to the [PHP Binary](#php_install_binary) section for instructions on installation.

#### Windows ####

Currently, only compilation against PHP 5.4.14 (x86) _Thread Safe_ version has been verified to work. Tested with Windows 7 Enterprise with Visual Studio 2008 SP1

__Compile bug work-around:__

Per a bug report at, https://bugs.php.net/bug.php?id=63130, and until a better solution is presented,
you should make the following changes to `malloc.h`:

This is a possible path where you might find it:

    C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\include\malloc.h

open it up as Administrator and change

~~~{.cpp}
#define _STATIC_ASSERT(expr) typedef char __static_assert_t[ (expr) ]
~~~
to
~~~{.cpp}
#ifdef PHP_WIN32
#define _STATIC_ASSERT(expr) typedef char __static_assert_t[ 1 ]
#else
#define _STATIC_ASSERT(expr) typedef char __static_assert_t[ (expr) ]
#endif
~~~

Run the `cmake-gui` utility to configure and generate the VS Solution or NMake Makefile.

I chose to specify `build` as the directory for output.

If all went well, building the solution should now provide a `php_gauss.dll` file in the `build\Release` directory.

### Binary ###

#### Windows / Linux ####

Place the `php_gauss.dll` (Windows) or `gauss.so` (Linux) in your PHP installation's `ext` directory.

This value can be found by either:

1. ``echo `php-config --extension-dir` `` on Linux
2. Looking for the `extension_dir` value in your `php.ini` file.

There is also the opportunity to load the extension directly by PHP, requiring the addition of the following to your `php.ini`:

    ; load gauss engine extension
    ; specify gauss.so if Linux
    extension=php_gauss.dll

If running PHP from the command line, changes should be instantaneous.

Please refresh services that utilize PHP to pick up on the new extension.

## Development ##

This section introduces an implementation of the GAUSS Engine.

### Hello World! ###

Getting started with the GAUSS Engine is extremely simple.

This assumes the library has been made available to the target language properly.

The following snippet will be described step-by-step in detail:

#### Python ####
~~~{.py}
from gauss import GAUSS, IGEProgramOutput

class Output(IGEProgramOutput):
    def invoke(self, message):
        print message,

ge = GAUSS()

out = Output()
out.thisown = 0
ge.setProgramOutputAll(out)

if not ge.initialize():
    print "Initialization failed."
    sys.exit(1)     # import sys for usage

ge.executeString("print \"Hello World!\"")
ge.shutdown()
~~~

#### PHP ####
~~~{.php}
include("gauss.php");

class Output extends IGEProgramOutput {
    function invoke($message) {
        echo $message;
    }
}

$ge = new GAUSS();

$out = new Output();
$out->thisown = 0;

$ge->setProgramOutputAll($out);

if (!$ge->initialize()) {
    echo "Initialization failed.";
    return;
}

$ge->executeString("print \"Hello World!\"");
$ge->shutdown();
~~~

### Step 1: Import the required library ###
Before we can make use of the functionality, we have to make the engine library available to us. This is quite simple and straight-forward.

#### Python ####
~~~{.py}
from gauss import GAUSS, IGEProgramOutput
~~~

#### PHP ####
~~~{.php}
include("gauss.php");
~~~

If you compiled from source and you're not sure where `gauss.php` is, recall that SWIG auto-generated it for us and it should be in the `src` directory.

### Step 2: Instantiate the GAUSS Engine object ###
After importing the correct library, we need to instantiate our engine object.
There are multiple constructors available that suit a variety of scenarios.

In this example, we will be using the default constructor.
This method relies on an existing environment variable in an effort to locate the GAUSS home directory.

By default, this environment variable is `MTENGHOME13`.

#### Python ####
~~~{.py}
ge = GAUSS()  # Look up value for MTENGHOME13
~~~
#### PHP ####
~~~{.php}
$ge = new GAUSS();  // Look up value for MTENGHOME13
~~~

__Hint:__ Check out the `GAUSS(string)` constructor, as it allows you to pass in a custom environment variable to use.

### Step 3: Set up callbacks ###

Technically, this step _can_ be skipped, but creating appropriate callbacks allows you control over specific aspects of program functionality.

While some of these must be implemented to work correctly (i.e. setting a string input callback), others have default implementations that don't
necessarily require a callback (i.e. program output routing to `stdout`), it is generally a good idea to do so.

The following 3 steps are core to setting up a callback:
1. Define the callback by deriving one of the available callback classes
2. Set the `thisown` property to __true__ (release ownership)
3. Assign the callback

Pretty straight-forward!

We first define a callback class, inheriting the appropriate base class; in this case IGEProgramOutput.

Simply override the callback method `invoke` and print out the `message` argument.

__Important Note:__

We must set the `thisown` property of the newly instantiated object to `0`

This is necessary because ownership of this object will be transferred to the GAUSS object.

Without setting this, if the `out` var was to go out of scope, it could be deleted.
When the engine attempts to clean up after itself, that object will no longer exist.

#### Python ####
~~~{.py}
# Step 1
class Output(IGEProgramOutput):
    def invoke(self, message):
        print message,

out = Output()

# Step 2
out.thisown = 0 # Prevent garbage collection

# Step 3
ge.setProgramOutputAll(out)
~~~
#### PHP ####
~~~{.php}
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
~~~

_Note:_ We used the GAUSS.setProgramOutputAll method, which is used to set _both_ the program and error output.

If you wish to separate the callbacks for different functionality, you can use the GAUSS.setProgramOutput
and GAUSS.setProgramErrorOutput methods.

### Step 4: Initialize the GAUSS Engine ###
Now that you've made it to this step, if all has been configured correctly, initializing the Engine is quite easy.

#### Python ####
~~~{.py}
if not ge.initialize():
    print "Initialization failed."
    sys.exit(1)
~~~
#### PHP ####
~~~{.php}
if (!$ge->initialize()) {
    echo "Initialization failed."
    return;
}
~~~

The `initialize` method returns a true/false value depending on whether it was successful or not.

### Step 5: Home Free ###
With everything set up, we are free to run whatever code we choose!

For this small example, we just want the engine to tell us "Hello World!" through the callback we've assigned.

#### Python ####
~~~{.py}
ge.executeString("print \"Hello World!\"")
~~~
#### PHP ####
~~~{.php}
$ge->executeString("print \"Hello World!\"");
~~~
will result in the output:
~~~
Hello World!
~~~

### Step 6: Shutdown the GAUSS Engine ###
Shutting down the GAUSS Engine is as easy as telling it to do so:

#### Python ####
~~~{.py}
ge.shutdown()
~~~
#### PHP ####
~~~{.php}
$ge->shutdown();
~~~

This will destroy any active workspaces and perform any cleanup that might be necessary.

## Conclusion ##
This wraps up the basics of using the GAUSS Engine API. The documentation is full of code snippets that should help you
easily interface with the Engine for your chosen task.

A template file has also been included which represents a full setup
and tear-down with space in the middle to drop in code snippets.
This file will be aptly named in relation to the language it applies to:

Language | Filename
:--------|:--------
Python   | `python/template_example.py`
PHP      | `php/template_example.php`
