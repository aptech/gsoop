#ifndef GAUSS_H
#define GAUSS_H

//#ifdef GAUSS_LIBRARY
//    #define DLL __attribute__((visibility("default"))) //__attribute__((dllexport)) // export
//#else
//    #define DLL __attribute__((dllimport)) // import
//#endif

#include <iostream>
#include <cstdlib>
#include <vector>
#include "mteng.h"
#include "gesymtype.h"
using namespace std;

class GEArray;
class GEMatrix;
class GEString;
class GEStringArray;
class GEWorkspace;
class WorkspaceManager;
class IGEProgramOutput;
class IGEProgramFlushOutput;
class IGEProgramInputString;
class IGEProgramInputChar;
class IGEProgramInputCheck;

/*! \mainpage Overview
 *
 * \section intro_sec Introduction
 *
 * Thank you for choosing the GAUSS Engine API.

This library is a set of target language bindings that allows
developers to intuitively and efficiently access the power of the GAUSS Engine.

To accomplish the task of providing these bindings for multiple languages,
we have created an initial C++ library and use the [SWIG](http://www.swig.org) program to generate
language-specific bindings.

While building from source allows complete control over
the compile process, we have elected to also release binaries for popular platforms.

You can easily and effectively:
+ Create and destroy workspaces, using multiple for threaded situations
+ Compile, save, load and execute programs.
+ Manipulate data in the symbol table
+ Set up callbacks for integration with GAUSS's input/output routines

\section getting_started_sec Configuration

There are a few things necessary to take care of before starting development.

The API will look for the `MTENGHOME13` environment variable.
This will be the location the engine was extracted to (i.e. `C:\mteng13` on Windows, `/home/user/mteng13` on Linux)

Variable          | Value
:-----------------|:-----------
`MTENGHOME13`     | `/home/user/mteng13`

The following environment variables must **contain** the specified values

\subsection windows_install Windows
Variable   | Value
:----------|:-----------
`PATH`     | Append `C:\mteng13`

The presence of this value allows the engine DLL to be found and loaded properly.

Ensure you do not __replace__ `PATH` with the directory.

\subsection linux_unix_install Linux and Unix

Variable          | Value
:-----------------|:-----------
`LD_LIBRARY_PATH` | Append `/home/user/mteng13`
`LD_PRELOAD`      | Append `/lib/x86_64-linux-gnu/libpthread.so.0:/home/user/mteng13/bin/libiomp5.so`

Note: Please adjust the paths accordingly to your specific installation. In this case, the `libpthread.so.0` reference is for Ubuntu 64-bit.

\subsection swig_config SWIG
If you are building from source, you will need to have the SWIG library available:

http://www.swig.org/download.html

Once installed, please ensure `swig` is available in your terminal by placing it's installation directory in your `PATH` environment variable, as building requires it.

__EXCEPTION__: If you are not *modifying* any source files, the auto generated files have been included in the respective `python` and `php` sub-directories.

You can place these in `src` and skip the steps involving SWIG.

\section installation Installation
This guide will not cover installation of the language itself. Please refer to the vendor documentation for language-level installation.

\subsection python_install Python
Installation of the Python module is fairly simple, since it uses the `distribute` package.

\subsubsection python_install_binary Binary

#### Windows ####

Select the correct `egg` file according to your architecture.

Please ensure your Python installation contains the `setuptools` package, which provides the `easy_install` application.

Instructions and files can be found at: https://pypi.python.org/pypi/setuptools#windows

    $ easy_install gauss-0.1-py2.7-win-amd64.egg

#### Linux ####

    $ easy_install gauss-0.1-py2.7-linux-x86_64.egg

\subsubsection python_install_source Source

Due to the nature of the Python installation on Windows, please ensure the following are correctly in your `PATH`
environment variable

    C:\Python27\;C:\Python27\Scripts\

Note: Tested with Python 2.7.4

Since we are building from source, we need to run SWIG on our interface file.

This is done automatically with the `setup.py` file, however, we must first build the extension.

By first calling `build_ext -i`, a `gauss.py` file is automatically generated in the current directory.

Subsequently calling the `install` target will now correctly install the files.

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

\subsection php_install PHP

\subsubsection php_install_source Source

[CMake](http://www.cmake.org) is our build tool of choice. Get it.

There are a couple steps common between both platforms that should be done ahead of time.

1. Set PHP specific environment variables.

    Our CMake file uses the `PHP_INCLUDE` environment variable to locate PHP headers.
    It expects a semi-colon delimited list of paths.

    For convenience, the Linux implementation does a string replace of '`-I`' with a semi-colon, so the following will work fine.

        # Linux
        $ export PHP_INCLUDE=`php-config --includes`

        # Windows you have to create it yourself. Newlines have been removed to clearly show the paths
        # The actual command should have them all appended to each one another separated by a semi-colon
        set PHP_INCLUDE=C:\development\php5.4ts\dev;
                        C:\development\php-sdk\php54dev\vc9\x86\php-5.4.14-src;
                        C:\development\php-sdk\php54dev\vc9\x86\php-5.4.14-src\main;
                        C:\development\php-sdk\php54dev\vc9\x86\php-5.4.14-src\Zend;
                        C:\development\php-sdk\php54dev\vc9\x86\php-5.4.14-src\TSRM;
                        C:\development\php-sdk\php54dev\vc9\x86\php-5.4.14-src\ext

    __Note__ the Windows line also has the `php5.4ts\dev` as part of the include path.

    This is so `php5ts.lib` can be located properly at link-time.
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

\subsubsection php_install_binary Binary

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

\section example_apps Development

This section introduces an implementation of the GAUSS Engine.

\subsection hello_world Hello World!

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

\subsection hw_step1 Step 1: Import the required library
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

\subsection hw_step2 Step 2: Instantiate the GAUSS Engine object
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

\subsection hw_step3 Step 3: Set up callbacks

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

\subsection hw_step4 Step 4: Initialize the GAUSS Engine
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

\subsection hw_step5 Step 5: Home Free
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

\subsection hw_step6 Step 6: Shutdown the GAUSS Engine
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

\subsection conc Conclusion
This wraps up the basics of using the GAUSS Engine API. The documentation is full of code snippets that should help you
easily interface with the Engine for your chosen task.

A template file has also been included which represents a full setup
and tear-down with space in the middle to drop in code snippets.
This file will be aptly named in relation to the language it applies to:

Language | Filename
:--------|:--------
Python   | `python/template_example.py`
PHP      | `php/template_example.php`

 */

/**
 * Main class that must be instantiated to interface with the GAUSS Engine.
 * While not enforced, only __one__ instance of this class should be present. If threading is an issue,
 * consider the use of multiple workspaces, as each can be accessed in parallel safely.
 *
 * #### Python ####
 * ~~~{.py}
ge = GAUSS() # Use MTENGHOME13 environment variable value as GAUSS home location
ge = GAUSS("MYHOMEVAR") # Use specified environment variable for GAUSS home location
ge = GAUSS("/home/user/mteng", False) # Use specified paths for GAUSS home location
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$ge = new GAUSS(); // Use MTENGHOME13 environment variable value as GAUSS home location
$ge = new GAUSS("MYHOMEVAR"); // Use specified environment variable for GAUSS home location
$ge = new GAUSS("/home/user/mteng", false); // Use specified paths for GAUSS home location
 * ~~~
 *
 */
class GAUSS {
public:
    GAUSS(void);
    GAUSS(string, bool isEnvVar = true);
    ~GAUSS();

    // init
    bool initialize();
    void shutdown();

    // home
    bool setHome(string);
    bool setHomeVar(string);
    string getHome();
    string getHomeVar();
    string getLogFile();
    bool setLogFile(string, string);

    // errors
    string getLastErrorText();
    string getErrorText(int);
    int getError();
    void setError(int);

    // workspaces
    GEWorkspace* createWorkspace(string);
    bool destroyWorkspace(GEWorkspace*);
    void destroyAllWorkspaces();
    GEWorkspace* getWorkspace(string);
    GEWorkspace* getActiveWorkspace();
    bool setActiveWorkspace(GEWorkspace*);
    GEWorkspace* loadWorkspace(string);
    string getWorkspaceName(GEWorkspace*);
    void updateWorkspaceName(GEWorkspace*);

    bool saveWorkspace(GEWorkspace*, string);
    bool saveProgram(ProgramHandle_t*, string);
    string translateDataloopFile(string srcfile);

    // program execution
    bool executeString(string);
    bool executeString(string, GEWorkspace*);
    bool executeFile(string);
    bool executeFile(string, GEWorkspace*);
    bool executeCompiledFile(string);
    bool executeCompiledFile(string, GEWorkspace*);
    ProgramHandle_t* compileString(string);
    ProgramHandle_t* compileString(string, GEWorkspace*);
    ProgramHandle_t* compileFile(string);
    ProgramHandle_t* compileFile(string, GEWorkspace*);
    ProgramHandle_t* loadCompiledFile(string);
    ProgramHandle_t* loadCompiledFile(string, GEWorkspace*);
    bool executeProgram(ProgramHandle_t*);
    void freeProgram(ProgramHandle_t*);

    string makePathAbsolute(string);
    string programInputString();
    int getSymbolType(string);
    int getSymbolType(string, GEWorkspace*);

    double getScalar(string);
    double getScalar(string, GEWorkspace*);
    GEMatrix* getMatrix(string);
    GEMatrix* getMatrix(string, GEWorkspace*);
    GEMatrix* getMatrixAndClear(string);
    GEMatrix* getMatrixAndClear(string, GEWorkspace*);
    GEArray* getArray(string);
    GEArray* getArray(string, GEWorkspace*);
    GEArray* getArrayAndClear(string);
    GEArray* getArrayAndClear(string, GEWorkspace*);
    GEString* getString(string);
    GEString* getString(string, GEWorkspace*);
    GEStringArray* getStringArray(string);
    GEStringArray* getStringArray(string, GEWorkspace*);

    bool setSymbol(GEMatrix*, string);
    bool setSymbol(GEMatrix*, string, GEWorkspace*);
    bool setSymbol(GEArray*, string);
    bool setSymbol(GEArray*, string, GEWorkspace*);
    bool setSymbol(GEString*, string);
    bool setSymbol(GEString*, string, GEWorkspace*);
    bool setSymbol(GEStringArray*, string);
    bool setSymbol(GEStringArray*, string, GEWorkspace*);

    bool isMissingValue(double);

    static double kMissingValue;

    static void internalHookOutput(char *output);
    static void internalHookError(char *output);
    static void internalHookFlush();
    static int internalHookInputString(char *buf, int len);
    static int internalHookInputChar();
    static int internalHookInputBlockingChar();
    static int internalHookInputCheck();

    void setProgramOutputAll(IGEProgramOutput *func);
    void setProgramOutput(IGEProgramOutput *func);
    void setProgramErrorOutput(IGEProgramOutput *func);
    void setProgramFlushOutput(IGEProgramFlushOutput *func);
    void setProgramInputString(IGEProgramInputString *func);
    void setProgramInputChar(IGEProgramInputChar *func);
    void setProgramInputCharBlocking(IGEProgramInputChar *func);
    void setProgramInputCheck(IGEProgramInputCheck *func);

    static IGEProgramOutput *outputFunc_;
    static IGEProgramOutput *errorFunc_;
    static IGEProgramFlushOutput *flushFunc_;
    static IGEProgramInputString *inputStringFunc_;
    static IGEProgramInputChar *inputCharFunc_;
    static IGEProgramInputChar *inputBlockingCharFunc_;
    static IGEProgramInputCheck *inputCheckFunc_;

private:
    void Init(string);

    // Keep these private, since we have accessors in place for these.
    void setHookOutput(void (*display_string_function)(char *str));
    void setHookProgramErrorOutput(void (*display_error_string_function)(char *));
    void setHookProgramOutput(void (*display_string_function)(char *str));
    void setHookFlushProgramOutput(void (*flush_display_function)(void));
    void setHookProgramInputChar(int (*get_char_function)(void));
    void setHookProgramInputBlockingChar(int (*get_char_blocking_function)(void));
    void setHookGetCursorPosition(int (*get_cursor_position_function)(void));
    void setHookProgramInputString(int (*get_string_function)(char *, int));
    void setHookProgramInputCheck(int (*get_string_function)(void));

    string gauss_home_;
    WorkspaceManager *manager_;

    char* removeConst(string*);

    Matrix_t* createTempMatrix(GEMatrix*);
    StringArray_t* createTempStringArray(GEStringArray*);
    String_t* createPermString(GEString*);
    Array_t* createPermArray(GEArray*);

    static string kHomeVar;
};


#endif // GAUSS_H
