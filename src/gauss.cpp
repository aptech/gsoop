#include "gauss.h"
#include <cstring>
#include <cctype>    // for isalnum()
#include <algorithm> // for back_inserter
#include "gearray.h"
#include "gematrix.h"
#include "gestring.h"
#include "gestringarray.h"
#include "geworkspace.h"
#include "workspacemanager.h"
#include "gefuncwrapper.h"
#include "pthread.h"
#ifdef _WIN32
#include "windows.h"
#else
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#endif

#include <stdio.h>
using namespace std;

#ifdef _WIN32
#  include <direct.h>
#  define getcwd _getcwd
#  define chdir _chrdir
#else
#  include <unistd.h>
#endif

/**
 * Bit pattern of a double missing value (NaN)
 */
double GAUSS::kMissingValue;

map<int, string> GAUSS::kOutputStore;
pthread_mutex_t GAUSS::kOutputMutex;
map<int, string> GAUSS::kErrorStore;
pthread_mutex_t GAUSS::kErrorMutex;

IGEProgramOutput* GAUSS::outputFunc_ = 0;
IGEProgramOutput* GAUSS::errorFunc_ = 0;
IGEProgramFlushOutput* GAUSS::flushFunc_ = 0;
IGEProgramInputString* GAUSS::inputStringFunc_ = 0;
IGEProgramInputChar* GAUSS::inputCharFunc_ = 0;
IGEProgramInputChar* GAUSS::inputBlockingCharFunc_ = 0;
IGEProgramInputCheck* GAUSS::inputCheckFunc_ = 0;

string GAUSS::kHomeVar = "MTENGHOME15";

int getThreadId() {
    int tid = -1;

#ifdef _WIN32
    tid = (int)GetCurrentThreadId();
#else
    tid = (int)syscall(SYS_gettid);
#endif

//    printf("thread id is %d\n", tid);
//    fflush(stdout);

    return tid;
}

/**
 * Initialize the library using the environment variable value of `MTENGHOME14` as the
 * path for the GAUSS Home path.
 *
 * @see GAUSS(string, bool)
 */
GAUSS::GAUSS(void)
{
    char *envVal = getenv(GAUSS::kHomeVar.c_str());

    string homeVal;

    if (envVal)
        homeVal = string(envVal);

    Init(homeVal);
}

/**
 * Initialize the library using the specified environment variable that contains the
 * path for the GAUSS Home directory. This is most useful in a
 * situation where the GAUSS Engine is being redistributed. This method allows the
 * developer to specify a custom environment variable that is specific to their product.
 * Note that the end user must have this environment variable set up as well.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
# Using a custom environment variable
ge = GAUSS("MY_CUSTOM_VAR");

# Setting a specific path
ge = GAUSS("/home/user/mteng", False);
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
// Using a custom environment variable
$ge = new GAUSS("MY_CUSTOM_VAR");

// Setting a specific path
$ge = new GAUSS("/home/user/mteng", false);
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
GAUSS ge = new GAUSS("MY_CUSTOM_VAR");
 * ~~~-->
 *
 * @param inp           Environment variable or absolute path to GAUSS Home (i.e. mteng.dll, libmteng.so)
 * @param isEnvVar        Whether the `inp` argument is an environment variable name
 *
 * @see        GAUSS()
 */
GAUSS::GAUSS(string inp, bool isEnvVar) {
    string homeVal = inp;

    if (isEnvVar) {
        char *envVal = getenv(inp.c_str());

        if (envVal)
            homeVal = string(envVal);
    }

    Init(homeVal);
}

void GAUSS::Init(string homePath) {
    this->gauss_home_ = homePath;
    this->manager_ = new WorkspaceManager;

    GAUSS::kMissingValue = GAUSS_MissingValue();

    GAUSS::outputFunc_ = 0;
    GAUSS::errorFunc_ = 0;
    GAUSS::flushFunc_ = 0;
    GAUSS::inputStringFunc_ = 0;
    GAUSS::inputCharFunc_ = 0;
    GAUSS::inputBlockingCharFunc_ = 0;
    GAUSS::inputCheckFunc_ = 0;

    if (homePath.empty()) {
        cerr << "Unable to find GAUSS Home directory. Aborting." << endl;
		cerr.flush();
		return;
    }
}

/**
 * Initialize the GAUSS Engine. This must be called before any other engine related functions are called.
 * This method informs the GAUSS Engine of the Home path that was set in the constructor, as well as creates
 * a default workspace. After this method succeeds the Engine is ready to handle symbols and compiling
 * and running programs.
 *
 * This is generally called after this object has been instantiated,
 * and directly following the setting of callbacks.
 *
 * @return true on success; false on failure
 *
 * @see shutdown()
 */
bool GAUSS::initialize() {
    if (!setHome(this->gauss_home_)) {
        string errorString = getLastErrorText();

        cerr << "Could not set GAUSS Home (Error: " << errorString << ")" << endl;
		cerr.flush();
        return false;
    }

    if (GAUSS_Initialize() >  0) {
        string errorString = getLastErrorText();

        cerr << "Could initialize GAUSS (Error: " << errorString << ")" << endl;
		cerr.flush();
        return false;
    }

    GEWorkspace *wh = createWorkspace("main");

    if (wh->workspace() == NULL) {
        string errorString = getLastErrorText();

        cerr << "Could not create workspace (Error: " << errorString << ")" << endl;
		cerr.flush();
        return false;
    }
    
    setActiveWorkspace(wh);

    setHookProgramOutput(GAUSS::internalHookOutput);
    setHookProgramErrorOutput(GAUSS::internalHookError);
    setHookFlushProgramOutput(GAUSS::internalHookFlush);
    setHookProgramInputString(GAUSS::internalHookInputString);
    setHookProgramInputChar(GAUSS::internalHookInputChar);
    setHookProgramInputBlockingChar(GAUSS::internalHookInputBlockingChar);
    setHookProgramInputCheck(GAUSS::internalHookInputCheck);

    return true;
}

/**
 * Free all created workspaces and shut down the GAUSS Engine. Note that the GAUSS Engine can be re-initialized
 * at run-time via another call to initialize().
 *
 * @see initialize()
 */
void GAUSS::shutdown() {
    destroyAllWorkspaces();

    GAUSS_Shutdown();
}

/**
 * Creates a new workspace with the given name, and returns a handle to it.
 *
 * Calling this does not replace the currently active workspace.
 *
 * @param name        Name of workspace to create
 * @return        Handle to newly created workspace
 *
 * @see setActiveWorkspace(GEWorkspace*)
 * @see destroyWorkspace(GEWorkspace*)
 */
GEWorkspace* GAUSS::createWorkspace(string name) {
    return this->manager_->create(name);
}

/**
 * Free a workspace. Deletes all associated symbols. This cannot be undone.
 *
 * Note that you will not be able to manipulate symbols without an active workspace.
 *
 * @param wh Workspace handle
 * @return Whether workspace was successfully removed
 *
 * @see createWorkspace(string)
 * @see destroyAllWorkspaces()
 */
bool GAUSS::destroyWorkspace(GEWorkspace *wh) {
    return this->manager_->destroy(wh);
}

/**
 * Clears all workspaces. Note that you will not be able to manipulate symbols
 * without an active workspace.
 *
 * @see createWorkspace(string)
 * @see destroyWorkspace(GEWorkspace*)
 */
void GAUSS::destroyAllWorkspaces() {
    this->manager_->destroyAll();
}

/**
 * Returns a handle to a specified workspace by _name_
 *
 * @param wkspName        Name of workspace
 * @return        Workspace handle
 *
 * @see setActiveWorkspace(GEWorkspace*)
 * @see getActiveWorkspace()
 */
GEWorkspace* GAUSS::getWorkspace(string name) {
    return this->manager_->getWorkspace(name);
}

/**
 * Return a handle to the currently active workspace.
 *
 * @return Active workspace objectut
 *
 * @see setActiveWorkspace(GEWorkspace*)
 */
GEWorkspace* GAUSS::getActiveWorkspace() {
    return this->manager_->getCurrent();
}

/**
 * Saves workspace information contained in a workspace handle into a file.
 * The file will have the name given by _fn_. Load the workspace information with loadWorkspace(string).
 *
 * @param wh        Workspace object
 * @param fn        Filename to save workspace as
 *
 * @see loadWorkspace(string)
 */
bool GAUSS::saveWorkspace(GEWorkspace *wh, string fn) {
    if (!manager_->isValidWorkspace(wh))
        return false;

    return (GAUSS_SaveWorkspace(wh->workspace(), removeConst(&fn)) == GAUSS_SUCCESS);
}

/**
 * Saves a compiled program given by a program handle into a file. It saves all of the
 * workspace information, which is contained in the program handle. The file will have
 * the name given by _fn_. Load the program with loadCompiledFile(string).
 *
 * @param ph        Program handle
 * @param fn        Filename to save program to
 * @return        True on success, false on failure
 *
 * @see loadCompiledFile(string)
 */
bool GAUSS::saveProgram(ProgramHandle_t *ph, string fn) {
    return (GAUSS_SaveProgram(ph, removeConst(&fn)) == GAUSS_SUCCESS);
}

/**
 * Sets the active workspace to be the specified workspace.
 *
 * @param wh        Workspace object
 *
 * @see getActiveWorkspace()
 */
bool GAUSS::setActiveWorkspace(GEWorkspace *wh) {
    return this->manager_->setCurrent(wh);
}

/**
 * Returns the current path known by the GAUSS Engine for the user home directory.
 *
 * @return        Path to user GAUSS home directory.
 *
 * @see setHome(string)
 */
string GAUSS::getHome() {
    char buf[1024];

    GAUSS_GetHome(buf);

    return string(buf);
}

/**
 * Returns the current environment variable value set by the GAUSS Engine that represents the GAUSS Home path.
 *
 * @return        Environment variable name
 *
 * @see setHomeVar(string)
 */
string GAUSS::getHomeVar() {
    char buf[1024];

    GAUSS_GetHomeVar(buf);

    return string(buf);
}

/**
 * Returns the current log file location used by the GAUSS Engine
 *
 * @return        Path to log file
 *
 * @see setLogFile(string, string)
 */
string GAUSS::getLogFile() {
    char buf[1024];

    GAUSS_GetLogFile(buf);

    return string(buf);
}

/**
 * Turns a given path into an absolute path.
 *
 * @param path     Path to be analyzed
 * @return      Absolute representation of _path_ argument
 */
string GAUSS::makePathAbsolute(string path) {
    char buf[4096];

    memset(buf, 0, sizeof(buf));

    strncpy(buf, path.c_str(), sizeof(buf));

    GAUSS_MakePathAbsolute(buf);

    return string(buf);
}

/**
 * Calls the program input string function hooked
 * with setProgramInputString(IGEProgramInputString*).
 *
 * The callbacks are thread specific. programInputString will call the
 * input string function that was hooked in that particular thread.
 *
 * @return        user input from hooked function
 *
 * @see setProgramInputString(IGEProgramInputString*)
 */
string GAUSS::programInputString() {
    char buf[4096];

    GAUSS_ProgramInputString(buf, 4096);

    return string(buf);
}

/**
 * Executes a command within the GAUSS Engine on the currently active workspace. If you wish to
 * run this command repeatedly, you can compile it first using compileString(string) and
 * then execute it as many times as you wish with executeProgram(ProgramHandle_t*).
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
ge.executeString("x = rndu(3,3)")
ge.executeString("print x")
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$ge->executeString("x = rndu(3,3)");
$ge->executeString("print x");
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
ge.executeString("x = rndu(3,3)");
ge.executeString("print x");
 * ~~~-->
 *
 * @param command Expression to execute.
 * @return true on success; false on failure
 *
 * @see compileString(string)
 */
bool GAUSS::executeString(string command) {
    return executeString(command, getActiveWorkspace());
}

/**
 * Executes a command within the GAUSS Engine on a specific workspace. If you wish to
 * run this command repeatedly, you can compile it first using compileString(string) and
 * then execute it as many times as you wish with executeProgram(ProgramHandle_t*).
 *
 * Example (Where `myWorkspace` is a GEWorkspace object):
 *
 * #### Python ####
 * ~~~{.py}
ge.executeString("x = rndu(3,3)", myWorkspace)
ge.executeString("print x", myWorkspace)
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$ge->executeString("x = rndu(3,3)", $myWorkspace);
$ge->executeString("print x", $myWorkspace);
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
ge.executeString("x = rndu(3,3)", myWorkspace);
ge.executeString("print x", myWorkspace);
 * ~~~-->
 *
 * @param command Expression to execute.
 * @return true on success; false on failure
 *
 * @see compileString(string)
 */
bool GAUSS::executeString(string command, GEWorkspace *wh) {
    if (!manager_->isValidWorkspace(wh))
        return false;

    ProgramHandle_t *ph = GAUSS_CompileString(wh->workspace(), removeConst(&command), 0, 0);

    if (!ph)
        return false;

    bool ret = executeProgram(ph);

    GAUSS_FreeProgram(ph);

    return ret;
}

/**
 * Executes a named file within the GAUSS Engine on the currently active workspace. If you wish to
 * run this file repeatedly, you can compile it first using compileFile(string) and
 * then execute it as many times as you wish with executeProgram(ProgramHandle_t*).
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
success = ge.executeFile("ols.e")
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$success = $ge->executeString("ols.e");
 * ~~~
 *
 * @param filename        Filename to execute.
 * @return true on success; false on failure
 *
 * @see compileFile(string)
 */
bool GAUSS::executeFile(string fname) {
    return executeFile(fname, getActiveWorkspace());
}

/**
 * Executes a named file within the GAUSS Engine on the a specific workspace. If you wish to
 * run this file repeatedly, you can compile it first using compileFile(string) and
 * then execute it as many times as you wish with executeProgram(ProgramHandle_t*).
 *
 * Example:
 *
 * Given _myWorkspace_ is a GEWorkspace object
 *
 * #### Python ####
 * ~~~{.py}
success = ge.executeFile("ols.e", myWorkspace)
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$success = $ge->executeString("ols.e", $myWorkspace);
 * ~~~
 *
 * @param filename        Filename to execute.
 * @return true on success; false on failure
 *
 * @see compileFile(string)
 */
bool GAUSS::executeFile(string fname, GEWorkspace *wh) {
    if (!manager_->isValidWorkspace(wh))
        return false;

    ProgramHandle_t *ph = GAUSS_CompileFile(wh->workspace(), removeConst(&fname), 0, 0);

    if (!ph)
        return false;

    bool ret = executeProgram(ph);

    GAUSS_FreeProgram(ph);

    return ret;
}

/**
 * Executes a compiled gcg file within the GAUSS Engine on the active workspace. As soon as
 * the file is finished executing it sets the current workspace to what it was before this function
 * was called. If you wish to run this file repeatedly, you can load it first using loadCompiledFile(string)
 * and then execute it as many times as you wish with executeProgram(ProgramHandle_t*).
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
success = ge.executeCompiledFile("example.gcg")
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$success = $ge->executeCompiledFile("example.gcg");
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
bool success = ge.executeCompiledFile("example.gcg");
 * ~~~-->
 *
 * @param filename        gcg file to execute.
 * @return true on success; false on failure
 *
 * @see loadWorkspace(string)
 * @see loadCompiledFile(string)
 */
bool GAUSS::executeCompiledFile(string fname) {
    return executeCompiledFile(fname, getActiveWorkspace());
}

/**
 * Executes a compiled gcg file within the GAUSS Engine on a specific workspace. As soon as
 * the file is finished executing it sets the current workspace to what it was before this function
 * was called. If you wish to run this file repeatedly, you can load it first using loadCompiledFile(string)
 * and then execute it as many times as you wish with executeProgram(ProgramHandle_t*).
 *
 * Example (Where `myWorkspace` is a GEWorkspace object):
 *
 * #### Python ####
 * ~~~{.py}
success = ge.executeCompiledFile("example.gcg", myWorkspace)
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$success = $ge->executeCompiledFile("example.gcg", $myWorkspace);
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
bool success = ge.executeCompiledFile("example.gcg", myWorkspace);
 * ~~~-->
 *
 * @param filename        gcg file to execute.
 * @return true on success; false on failure
 *
 * @see loadWorkspace(string)
 * @see loadCompiledFile(string)
 */
bool GAUSS::executeCompiledFile(string fname, GEWorkspace *wh) {
    if (!manager_->isValidWorkspace(wh))
        return false;

    ProgramHandle_t *ph = GAUSS_LoadCompiledFile(wh->workspace(), removeConst(&fname));

    if (!ph)
        return false;

    bool ret = executeProgram(ph);

    GAUSS_FreeProgram(ph);

    return ret;
}

/**
 * Compiles a string and returns a program handle in the active workspace. This can then be followed with a call
 * to executeProgram(ProgramHandle_t*). Note that if you do not care about keeping the program handle,
 * a convenience method executeString(string) is available.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
ph = ge.compileString("print \"Hello World!\"")

for i in range(0, 5):
    ge.executeProgram(ph)
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$ph = $ge->compileString("print \"Hello World!\"");

for ($i = 0; $i < 5; ++$i)
    $ge->executeProgram($ph);
 * ~~~
 * results in output:
 * ~~~
Hello World!
Hello World!
Hello World!
Hello World!
Hello World!
 * ~~~
 *
 * @param command        Code to compile
 * @return        Program handle
 *
 * @see executeProgram(ProgramHandle_t*)
 * @see executeString(string)
 * @see freeProgram
 */
ProgramHandle_t* GAUSS::compileString(string command) {
    return compileString(command, getActiveWorkspace());
}

/**
 * Compiles a string and returns a program handle in the specified workspace. This can then be followed with a call
 * to executeProgram(ProgramHandle_t*). Note that if you do not care about keeping the program handle,
 * a convenience method executeString(string) is available.
 *
 * Example:
 *
 * Given `myWorkspace` is a GEWorkspace object
 *
 * #### Python ####
 * ~~~{.py}
ph = ge.compileString("print \"Hello World!\"", myWorkspace)

for i in range(0, 5):
    ge.executeProgram(ph)
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$ph = $ge->compileString("print \"Hello World!\"", $myWorkspace);

for ($i = 0; $i < 5; ++$i)
    $ge->executeProgram($ph);
 * ~~~
 * results in output:
 * ~~~
Hello World!
Hello World!
Hello World!
Hello World!
Hello World!
 * ~~~
 *
 * @param command        Code to compile
 * @return        Program handle
 *
 * @see executeProgram(ProgramHandle_t*)
 * @see executeString(string)
 * @see freeProgram
 */
ProgramHandle_t* GAUSS::compileString(string command, GEWorkspace *wh) {
    if (!manager_->isValidWorkspace(wh))
        return NULL;

    return GAUSS_CompileString(wh->workspace(), removeConst(&command), 0, 0);
}

/**
 * Compiles a file and returns a program handle in the active workspace. This can then be followed with a call
 * to executeProgram(ProgramHandle_t*). Note that if you do not care about keeping the program handle,
 * a convenience method executeFile(string) is available.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
ph = ge.compileFile("ols.e")
ge.executeProgram(ph)
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$ph = $ge->compileFile("ols.e");
$ge->executeProgram(ph);
 * ~~~
 *
 * @param fn        Filename to compile
 * @return        Program handle
 *
 * @see executeProgram(ProgramHandle_t*)
 * @see executeFile(string)
 * @see freeProgram
 */
ProgramHandle_t* GAUSS::compileFile(string fname) {
    return compileFile(fname, getActiveWorkspace());
}

/**
 * Compiles a file and returns a program handle in a specific workspace. This can then be followed with a call
 * to executeProgram(ProgramHandle_t*). Note that if you do not care about keeping the program handle,
 * a convenience method executeFile(string) is available.
 *
 * Example:
 *
 * Given `myWorkspace` is a GEWorkspace object
 *
 * #### Python ####
 * ~~~{.py}
ph = ge.compileFile("ols.e", myWorkspace)
ge.executeProgram(ph)
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$ph = $ge->compileFile("ols.e", $myWorkspace);
$ge->executeProgram(ph);
 * ~~~
 *
 * @param fn        Filename to compile
 * @return        Program handle
 *
 * @see executeProgram(ProgramHandle_t*)
 * @see executeFile(string)
 */
ProgramHandle_t* GAUSS::compileFile(string fname, GEWorkspace *wh) {
    if (!manager_->isValidWorkspace(wh))
        return NULL;

    return GAUSS_CompileFile(wh->workspace(), removeConst(&fname), 0, 0);
}

/**
 * Loads an already compiled file into the active workspace and returns a program handle. This can then
 * be followed with a call to executeProgram(ProgramHandle_t*). Note that if you do not care about
 * keeping the program handle, a convenience method executeCompiledFile(string) is available.
 *
 * @param fn        Filename to load
 * @return        Program handle
 *
 * @see executeProgram(ProgramHandle_t*)
 * @see executeCompiledFile(string)
 */
ProgramHandle_t* GAUSS::loadCompiledFile(string fn) {
    return loadCompiledFile(fn, getActiveWorkspace());
}

/**
 * Loads an already compiled file into a specific workspace and returns a program handle. This can then
 * be followed with a call executeProgram(ProgramHandle_t*). Note that if you do not care about
 * keeping the program handle, a convenience method executeCompiledFile(string) is available.
 *
 * @param fn        Filename to load
 * @return        Program handle
 *
 * @see executeProgram(ProgramHandle_t*)
 * @see executeCompiledFile(string)
 */
ProgramHandle_t* GAUSS::loadCompiledFile(string fn, GEWorkspace *wh) {
    if (!manager_->isValidWorkspace(wh))
        return NULL;

    return GAUSS_LoadCompiledFile(wh->workspace(), removeConst(&fn));
}

/**
 * Executes a given program handle that was created with either compileString(string), compileFile(string),
 * or loadCompiledFile(string).
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
ph = ge.compileString("x = rndu(3,3)")
ge.executeProgram(ph)
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$ph = $ge->compileString("x = rndu(3,3)");
$ge->executeProgram($ph);
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
Object ph = ge.compileString("x = rndu(3,3)");
ge.executeProgram(ph);
 * ~~~-->
 *
 * @param ph Program handle
 * @return        True on success. False on failure
 *
 * @see compileString(string)
 * @see compileFile(string)
 * @see loadCompiledFile(string)
 * @see freeProgram(ProgramHandle_t*)
 */
bool GAUSS::executeProgram(ProgramHandle_t *ph) {
    if (!ph)
        return false;

    // Setup output hook
    setHookProgramOutput(GAUSS::internalHookOutput);
    setHookProgramErrorOutput(GAUSS::internalHookError);

	if (GAUSS_Execute(ph) > 0) {
        return false;
	}

    return true;
}

/**
 * Gets the workspace information saved in a file and
 * returns it in a workspace handle. This also sets the loaded workspace
 * to be the current workspace.
 *
 * If _loadWorkspace_ fails, _wh_ will be `null`. Use getError() to
 * get the number of the error. _loadWorkspace_ may fail with either of
 * the following errors:
 *
 * Error | Reason
 * -----:|------------
 * 30    | Insufficient memory.
 * 495   | Workspace inactive or corrupt.
 *
 * @param gcgfile        name of file workspace is stored in
 * @return        pointer to a workspace handle.
 */
GEWorkspace* GAUSS::loadWorkspace(string gcgfile) {
    WorkspaceHandle_t *wh = GAUSS_LoadWorkspace(removeConst(&gcgfile));

    if (!wh)
        return NULL;

    GEWorkspace *newWh = new GEWorkspace(wh);

    updateWorkspaceName(newWh);

    // set as active workspace
    setActiveWorkspace(newWh);

    return newWh;
}

/**
 * Returns the current associated name of a workspace according to GAUSS
 *
 * @param wh Workspace handle
 * @return        Workspace name
 *
 * @see getActiveWorkspace()
 */
string GAUSS::getWorkspaceName(GEWorkspace *wh) {
    if (!manager_->isValidWorkspace(wh))
        return string();

    char name[1024];
    memset(&name, 0, sizeof(name));

    GAUSS_GetWorkspaceName(wh->workspace(), name);

    return string(name);
}

/**
 * Request the workspace name from GAUSS, and set the _name_
 * field in the GEWorkspace object.
 *
 * @param wh        Workspace handle
 *
 * @see getWorkspaceName(GEWorkspace*)
 */
void GAUSS::updateWorkspaceName(GEWorkspace *wh) {
    string wkspName = getWorkspaceName(wh);

    wh->setName(wkspName);
}

/**
 * Free a program handle created by compileString(string),
 * compileFile(string), and loadCompiledFile(string)
 *
 * @param ph        Program handle
 *
 * @see compileString(string)
 * @see compileFile(string)
 * @see loadCompiledFile(string)
 * @see executeProgram(ProgramHandle_t*)
 */
void GAUSS::freeProgram(ProgramHandle_t *ph) {
    GAUSS_FreeProgram(ph);
}

/**
 * Returns the type of a symbol in the active GAUSS workspace or 0 if it cannot find the symbol.
 * Valid return types are constant members of the GESymType class
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.php}
if ge.getSymbolType("x") == GESymType.MATRIX:
    doSomething()
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
if ($ge->getSymbolType("x") == GESymType::MATRIX)
    doSomething();
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
if (ge.getSymbolType("x") == GESymType.MATRIX)
    doSomething();
 * ~~~->
 *
 * @param name        Name of GAUSS Symbol
 * @return int that represents symbol type. Refer to GESymType const list.
 *
 * @see GESymType.ARRAY_GAUSS
 * @see GESymType.MATRIX
 * @see GESymType.SCALAR
 * @see GESymType.SPARSE
 * @see GESymType.STRING
 * @see GESymType.STRING_ARRAY
 */
int GAUSS::getSymbolType(string name) {
    return getSymbolType(name, getActiveWorkspace());
}

/**
 * Returns the type of a symbol in a GAUSS workspace or 0 if it cannot find the symbol.
 * Valid return types are constant members of the GESymType class
 *
 * Example, given `myWorkspace` is a GEWorkspace object:
 *
 * #### Python ####
 * ~~~{.py}
if ge.getSymbolType("x", myWorkspace) == GESymType.MATRIX:
    doSomething()
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
if ($ge->getSymbolType("x", $myWorkspace) == GESymType::MATRIX)
    doSomething();
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
if (ge.getSymbolType("x", myWorkspace) == GESymType.MATRIX)
    doSomething();
 * ~~~-->
 *
 * @param name        Name of GAUSS Symbol
 * @return int that represents symbol type. Refer to GESymType const list.
 *
 * @see GESymType.ARRAY_GAUSS
 * @see GESymType.MATRIX
 * @see GESymType.SCALAR
 * @see GESymType.SPARSE
 * @see GESymType.STRING
 * @see GESymType.STRING_ARRAY
 */
int GAUSS::getSymbolType(string name, GEWorkspace *wh) {
    if (!manager_->isValidWorkspace(wh))
        return -1;

    return GAUSS_GetSymbolType(wh->workspace(), removeConst(&name));
}

/**
 * Manually set the current error code for the GAUSS Engine. Setting this to 0
 * will clear out the current error code.
 *
 * @param errNum        Error code
 *
 * @see getError()
 * @see getErrorText(int)
 * @see getLastErrorText()
 */
void GAUSS::setError(int errorNum) {
    GAUSS_SetError(errorNum);
}

char* GAUSS::removeConst(string *str) {
    return const_cast<char*>(str->c_str());
}

/**
 * Allows you to set the file that the errors will be logged in.
 * The Engine logs certain system level errors in 2 places: a
 * file and an open file pointer. The default file is `/tmp/mteng.###.log` where
 * `###` is the process ID number. The default file pointer is stderr.
 *
 * You can turn off the error logging to file by inputting an empty string for _logfn_.
 *
 * @param logfn        name of log file.
 * @param mode        **w** to overwrite the contents of the file.\n **a** to append to the contents of the file.
 * @return        True on success, false on failure
 *
 * @see getLogFile()
 */
bool GAUSS::setLogFile(string logfn, string mode) {
    char *logfn_ptr = removeConst(&logfn);

    if (logfn.empty())
        logfn_ptr = 0;

    return GAUSS_SetLogFile(logfn_ptr, removeConst(&mode)) == GAUSS_SUCCESS;
}

/**
 * Specifies the home directory used to locate the Run-Time Library, source files,
 * library files, etc. in a normal engine installation. It overrides any environment
 * variable.
 *
 * @param path        Directory path of engine installation
 * @return        True on success, false on failure
 *
 * @see getHome()
 * @see setHomeVar(string)
 */
bool GAUSS::setHome(string path) {
    return GAUSS_SetHome(removeConst(&path)) == GAUSS_SUCCESS;
}

/**
 * The default value is `MTENGHOME##`. Use the Java library function to get the value of the environment variable.
 *
 * It is better to use setHome(string) which sets the home directory, overriding
 * the environment variable.
 *
 * @param envVar        Name of environment variable
 * @return        True on success, false on failure
 *
 * @see getHomeVar()
 * @see setHome(string)
 */
bool GAUSS::setHomeVar(string envVar) {
    return GAUSS_SetHomeVar(removeConst(&envVar)) == GAUSS_SUCCESS;
}

/**
 * Return the string error description of the last error code.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
if not ge.initialize():
    print "Error initializing: " + ge.getLastErrorText()
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
if (!$ge->initialize()) {
    echo "Error initializing: " . $ge->getLastErrorText() . PHP_EOL;
}
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
if (!ge.initialize()) {
    System.out.println("Error initializing: " + ge.getLastErrorText());
}
 * ~~~-->
 *
 * @return        Description of last GAUSS error code
 *
 * @see getError()
 * @see getErrorText(int)
 */
string GAUSS::getLastErrorText() {
    int errNum = getError();

    return getErrorText(errNum);
}

/**
 * Retrieve the last error code set by the GAUSS Engine. You can retrieve a textual representation
 * of this error code by calling getErrorText(int) and passing in this value, or use the
 * getLastErrorText() method provided that does both.
 *
 * @return        Last error code
 *
 * @see getErrorText(int)
 * @see getLastErrorText()
 */
int GAUSS::getError() {
    return GAUSS_GetError();
}

/**
 * Given a GAUSS error code, return the full error description. If you would like
 * the error description of the last error that occurred, you can use getLastErrorText()
 * instead.
 *
 * @param errorNum        Error number
 * @return                        Complete error description
 *
 * @see getError()
 * @see getLastErrorText()
 */
string GAUSS::getErrorText(int errorNum) {
    char buf[1024];

    GAUSS_ErrorText(buf, errorNum);

    return string(buf);
}

/**
 * Check a double to see if it contains a GAUSS missing value
 *
 * @param d        double value to check
 * @return        true if <i>d</i> contains a GAUSS missing value. false if not.
 *
 * @see kMissingValue
 */
bool GAUSS::isMissingValue(double d) {
    return (GAUSS_IsMissingValue(d) > 0);
}

/**
 * Returns a scalar from the current workspace as a primitive `double`.
 * If the symbol _name_ does not exist, 0 will be returned.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
ge.executeString("x = 5")
x = ge.getScalar("x")
print "x = " + str(x)
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$ge->executeString("x = 5");
$x = $ge->getScalar("x");
echo "\$x = " . $x . PHP_EOL;
 * ~~~
 * will result in the output:
 * ~~~
$x = 5
 * ~~~
 *
 * @param name        Name of symbol
 * @return        Scalar object representing GAUSS symbol
 *
 * @see getScalar(string, GEWorkspace*)
 * @see setSymbol(GEMatrix*, string)
 * @see setSymbol(GEMatrix*, string, GEWorkspace*)
 * @see getMatrixAndClear(string)
 * @see getMatrixAndClear(string, GEWorkspace*)
 */
double GAUSS::getScalar(string name) {
    return getScalar(name, getActiveWorkspace());
}

/**
 * Returns a scalar from a specific workspace as a primitive `double`.
 * If the symbol _name_ does not exist, 0 will be returned.
 *
 * Example:
 *
 * Given _myWorkspace_ is a GEWorkspace object.
 *
 * #### Python ####
 * ~~~{.py}
ge.executeString("x = 5", myWorkspace)
x = ge.getScalar("x", myWorkspace)
print "x = " + str(x)
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$ge->executeString("x = 5", $myWorkspace);
$x = $ge->getScalar("x", $myWorkspace);
echo "\$x = " . $x . PHP_EOL;
 * ~~~
 * will result in the output:
 * ~~~
$x = 5
 * ~~~
 *
 * @param name        Name of symbol
 * @return        Scalar object representing GAUSS symbol
 *
 * @see getScalar(string)
 * @see setSymbol(GEMatrix*, string)
 * @see setSymbol(GEMatrix*, string, GEWorkspace*)
 * @see getMatrixAndClear(string)
 * @see getMatrixAndClear(string, GEWorkspace*)
 */
double GAUSS::getScalar(string name, GEWorkspace *wh) {
    if (!manager_->isValidWorkspace(wh))
        return 0;

    double d;

    int ret = GAUSS_GetDouble(wh->workspace(), &d, removeConst(&name));

    if (ret > 0) {
        return 0;
    }

    return d;
}

/**
 * Retrieve a matrix from the GAUSS symbol name in the active workspace. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without
 * first calling setSymbol(GEMatrix*, string).
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
ge.executeString("x = 5")
x = ge.getMatrix("x")
print "x = " + str(x.getElement())
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$ge->executeString("x = 5");
$x = $ge->getMatrix("x");
echo "x = " . $x->getElement() . PHP_EOL;
 * ~~~
 * will result in the output:
 * ~~~
x = 5
 * ~~~
 *
 * @param name        Name of GAUSS symbol
 * @return        Matrix object
 *
 * @see getMatrix(string, GEWorkspace*)
 * @see setSymbol(GEMatrix*, string)
 * @see setSymbol(GEMatrix*, string, GEWorkspace*)
 * @see getMatrixAndClear(string)
 * @see getMatrixAndClear(string, GEWorkspace*)
 * @see getScalar(string)
 * @see getScalar(string, GEWorkspace*)
 */
GEMatrix* GAUSS::getMatrix(string name) {
    return getMatrix(name, getActiveWorkspace());
}

/**
 * Retrieve a matrix from the GAUSS symbol name in workspace _wh_. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without
 * first calling setSymbol(GEMatrix*, string).
 *
 * Example:
 *
 * Given _myWorkspace_ is a GEWorkspace object.
 *
 * #### Python ####
 * ~~~{.py}
ge.executeString("x = 5", myWorkspace)
x = ge.getMatrix("x", myWorkspace)
print "x = " + str(x.getElement())
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$ge->executeString("x = 5", $myWorkspace);
$x = $ge->getMatrix("x", $myWorkspace);
echo "x = " . $x->getElement() . PHP_EOL;
 * ~~~
 * will result in the output:
 * ~~~
x = 5
 * ~~~
 *
 * @param name        Name of GAUSS symbol
 * @return        Matrix object
 *
 * @see getMatrix(string)
 * @see setSymbol(GEMatrix*, string)
 * @see setSymbol(GEMatrix*, string, GEWorkspace*)
 * @see getMatrixAndClear(string)
 * @see getMatrixAndClear(string, GEWorkspace*)
 * @see getScalar(string)
 * @see getScalar(string, GEWorkspace*)
 */
GEMatrix* GAUSS::getMatrix(string name, GEWorkspace *wh) {
    if (!manager_->isValidWorkspace(wh))
        return NULL;

    Matrix_t *gsMat = GAUSS_GetMatrix(wh->workspace(), removeConst(&name));

    if (gsMat == NULL) {
        return NULL;
    }

    return new GEMatrix(gsMat);
}

/**
 * Retrieve a matrix from the GAUSS symbol name in the active workspace. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without
 * first calling setSymbol(GEMatrix*, string).
 *
 * In addition, this function will clear the symbol from the GAUSS symbol table.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
ge.executeString("x = 5")
x = ge.getMatrixAndClear("x")
print "$x = " + str(x.getElement())
ge.executeString("print \"x = \" x")
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$ge->executeString("x = 5");
$x = $ge->getMatrixAndClear("x");
echo "\$x = " . $x->getElement() . PHP_EOL;
$ge->executeString("print \"x = \" x");
 * ~~~
 * will result in the output:
 * ~~~
$x = 5
x =        0.0000000
 * ~~~
 *
 * @param name        Name of GAUSS symbol
 * @return        Matrix object
 *
 * @see getMatrixAndClear(string, GEWorkspace*)
 * @see setSymbol(GEMatrix*, string)
 * @see setSymbol(GEMatrix*, string, GEWorkspace*)
 * @see getMatrix(string)
 * @see getMatrix(string, GEWorkspace*)
 * @see getScalar(string)
 * @see getScalar(string, GEWorkspace*)
 */
GEMatrix* GAUSS::getMatrixAndClear(string name) {
    return getMatrixAndClear(name, getActiveWorkspace());
}

/**
 * Retrieve a matrix from the GAUSS symbol name in workspace _wh_. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without
 * first calling setSymbol(GEMatrix*, string).
 *
 * In addition, this function will clear the symbol from the GAUSS symbol table.
 *
 * Example:
 *
 * Given _myWorkspace_ is a GEWorkspace object
 *
 * #### Python ####
 * ~~~{.py}
ge.executeString("x = 5", myWorkspace)
x = ge.getMatrixAndClear("x", myWorkspace)
print "\$x = " + str(x.getElement())
ge.executeString("print \"x = \" x", myWorkspace);
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$ge->executeString("x = 5", $myWorkspace);
$x = $ge->getMatrixAndClear("x", $myWorkspace);
echo "\$x = " . $x->getElement() . PHP_EOL;
$ge->executeString("print \"x = \" x", $myWorkspace);
 * ~~~
 * will result in the output:
 * ~~~
$x = 5
x =        0.0000000
 * ~~~
 *
 * @param name        Name of GAUSS symbol
 * @return        Matrix object
 *
 * @see getMatrixAndClear(string)
 * @see setSymbol(GEMatrix*, string)
 * @see setSymbol(GEMatrix*, string, GEWorkspace*)
 * @see getMatrix(string)
 * @see getMatrix(string, GEWorkspace*)
 * @see getScalar(string)
 * @see getScalar(string, GEWorkspace*)
 */
GEMatrix* GAUSS::getMatrixAndClear(string name, GEWorkspace *wh) {
    if (!manager_->isValidWorkspace(wh))
        return NULL;

    Matrix_t *gsMat = GAUSS_GetMatrixAndClear(wh->workspace(), removeConst(&name));

    if (gsMat == NULL)
        return NULL;

    return new GEMatrix(gsMat);
}

/**
 * Retrieve an array from the GAUSS symbol table in the active workspace. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without first
 * calling setSymbol(GEArray*, string).
 *
 * @param name        Name of GAUSS symbol
 * @return        Array object
 *
 * @see getArray(string, GEWorkspace*)
 * @see setSymbol(GEArray*, string)
 * @see setSymbol(GEArray*, string, GEWorkspace*)
 * @see getArrayAndClear(string)
 * @see getArrayAndClear(string, GEWorkspace*)
 */
GEArray* GAUSS::getArray(string name) {
    return getArray(name, getActiveWorkspace());
}

/**
 * Retrieve an array from the GAUSS symbol table in workspace _wh_. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without first
 * calling setSymbol(GEArray*, string).
 *
 * @param name        Name of GAUSS symbol
 * @return        Array object
 *
 * @see getArray(string)
 * @see setSymbol(GEArray*, string)
 * @see setSymbol(GEArray*, string, GEWorkspace*)
 * @see getArrayAndClear(string)
 * @see getArrayAndClear(string, GEWorkspace*)
 */
GEArray* GAUSS::getArray(string name, GEWorkspace *wh) {
    if (!manager_->isValidWorkspace(wh))
        return NULL;

    Array_t *gsArray = GAUSS_GetArray(wh->workspace(), removeConst(&name));

    if (gsArray == NULL)
        return NULL;

    return new GEArray(gsArray);
}

/**
 * Retrieve an array from the GAUSS symbol table in the active workspace. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without first
 * calling setSymbol(GEArray*, string).
 *
 * In addition, this function will clear the symbol from the GAUSS symbol table.
 *
 * @param name        Name of GAUSS symbol
 * @return        Array object
 *
 * @see getArrayAndClear(string, GEWorkspace*)
 * @see setSymbol(GEArray*, string)
 * @see setSymbol(GEArray*, string, GEWorkspace*)
 * @see getArray(string)
 * @see getArray(string, GEWorkspace*)
 */
GEArray* GAUSS::getArrayAndClear(string name) {
    return getArrayAndClear(name, getActiveWorkspace());
}

/**
 * Retrieve an array from the GAUSS symbol table in workspace _wh_. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without first
 * calling setSymbol(GEArray*, string).
 *
 * In addition, this function will clear the symbol from the GAUSS symbol table.
 *
 * @param name        Name of GAUSS symbol
 * @return        Array object
 *
 * @see getArrayAndClear(string)
 * @see setSymbol(GEArray*, string)
 * @see setSymbol(GEArray*, string, GEWorkspace*)
 * @see getArray(string)
 * @see getArray(string, GEWorkspace*)
 */
GEArray* GAUSS::getArrayAndClear(string name, GEWorkspace *wh) {
    if (!manager_->isValidWorkspace(wh))
        return NULL;

    Array_t *gsArray = GAUSS_GetArrayAndClear(wh->workspace(), removeConst(&name));

    if (gsArray == NULL)
        return NULL;

    return new GEArray(gsArray);
}

/**
 * Retrieve a string array from the GAUSS symbol table in the active workspace. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without first
 * calling setSymbol(GEStringArray*, string).
 *
 * @param name        Name of GAUSS symbol
 * @return        string array object
 *
 * @see getStringArray(string, GEWorkspace*)
 * @see setSymbol(GEStringArray*, string)
 * @see setSymbol(GEStringArray*, string, GEWorkspace*)
 */
GEStringArray* GAUSS::getStringArray(string name) {
    return getStringArray(name, getActiveWorkspace());
}

/**
 * Retrieve a string array from the GAUSS symbol table in workspace _wh_. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without first
 * calling setSymbol(GEStringArray*, string).
 *
 * @param name        Name of GAUSS symbol
 * @return        string array object
 *
 * @see getStringArray(string, GEWorkspace*)
 * @see setSymbol(GEStringArray*, string)
 * @see setSymbol(GEStringArray*, string, GEWorkspace*)
 */
GEStringArray* GAUSS::getStringArray(string name, GEWorkspace *wh) {
    if (!manager_->isValidWorkspace(wh))
        return NULL;

    StringArray_t *gsStringArray = GAUSS_GetStringArray(wh->workspace(), removeConst(&name));

    if (gsStringArray == NULL)
        return NULL;

    return new GEStringArray(gsStringArray);
}

/**
 * Retrieve a string from the GAUSS symbol table in the active workspace. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without
 * first calling setSymbol(GEString*, string).
 *
 * @param name        Name of GAUSS symbol
 * @return        string object
 *
 * @see getString(string, GEWorkspace*)
 * @see setSymbol(GEString*, string)
 * @see setSymbol(GEString*, string, GEWorkspace*)
 */
GEString* GAUSS::getString(string name) {
    return getString(name, getActiveWorkspace());
}

/**
 * Retrieve a string from the GAUSS symbol table in workspace _wh_. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without
 * first calling setSymbol(GEString*, string).
 *
 * @param name        Name of GAUSS symbol
 * @return        string object
 *
 * @see getString(string)
 * @see setSymbol(GEString*, string)
 * @see setSymbol(GEString*, string, GEWorkspace*)
 */
GEString* GAUSS::getString(string name, GEWorkspace *wh) {
    if (!manager_->isValidWorkspace(wh))
        return NULL;

    String_t *gsString = GAUSS_GetString(wh->workspace(), removeConst(&name));

    if (gsString == NULL)
        return NULL;

    return new GEString(gsString);
}

/**
 * Add a matrix to the active workspace with the specified symbol name.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
x = GEMatrix(5.0)
ge.setSymbol(x, "x")
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$x = new GEMatrix(5);
$ge->setSymbol($x, "x");
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
GEMatrix x = new GEMatrix(5);
ge.setSymbol(x, "x");
 * ~~~->
 *
 * @param matrix    Matrix object to store in GAUSS symbol table
 * @param name      Name to give newly added symbol
 * @return          True on success, false on failure
 *
 * @see setSymbol(GEMatrix*, string, GEWorkspace*)
 * @see getMatrix(string)
 * @see getMatrixAndClear(string)
 * @see getScalar(string)
 */
bool GAUSS::setSymbol(GEMatrix *matrix, string name) {
    return setSymbol(matrix, name, getActiveWorkspace());
}

/**
 * Add a matrix to a specific workspace with the specified symbol name.
 *
 * Example:
 *
 * Given _myWorkspace_ is a GEWorkspace object
 *
 * #### PHP ####
 * ~~~{.php}
x = GEMatrix(5.0)
ge.setSymbol(x, "x", myWorkspace)
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$x = new GEMatrix(5);
$ge->setSymbol($x, "x", $myWorkspace);
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
GEMatrix x = new GEMatrix(5);
ge.setSymbol(x, "x", myWorkspace);
 * ~~~-->
 *
 * @param matrix    Matrix object to store in GAUSS symbol table
 * @param name      Name to give newly added symbol
 * @return          True on success, false on failure
 *
 * @see setSymbol(GEMatrix*, string)
 * @see getMatrix(string)
 * @see getMatrixAndClear(string)
 * @see getScalar(string)
 */
bool GAUSS::setSymbol(GEMatrix *matrix, string name, GEWorkspace *wh) {
    if (!matrix || name.empty())
        return false;

    if (!manager_->isValidWorkspace(wh))
        return false;

    int ret = 0;

    if (!matrix->isComplex() && (matrix->getRows() == 1) && (matrix->getCols() == 1)) {
        ret = GAUSS_PutDouble(wh->workspace(), matrix->getElement(), removeConst(&name));
    } else {
        Matrix_t* newMat = this->createTempMatrix(matrix);
        ret = GAUSS_CopyMatrixToGlobal(wh->workspace(), newMat, removeConst(&name));
        delete newMat;
    }

    return (ret == GAUSS_SUCCESS);
}

/**
 * Add an array to the active workspace with the specified symbol name.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
orders = [2, 2, 2]
data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]
a = GEArray(orders, data)
ge.setSymbol(a, "a")
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$orders = array(2, 2, 2);
$data = array(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
$a = new GEArray($orders, $data);
$ge->setSymbol($a, "a");
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
int[] orders = new int[] { 2, 2, 2 };
double[] data = new double[] { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0 };
GEArray a = new GEArray(orders, data);
ge.setSymbol(a, "a");
 * ~~~-->
 *
 * @param array        Array object to store in GAUSS symbol table
 * @param name        Name to give newly added symbol
 * @return True on success, false on failure
 *
 * @see setSymbol(GEArray*, string, GEWorkspace*)
 * @see getArray(string)
 * @see getArrayAndClear(string)
 */
bool GAUSS::setSymbol(GEArray *array, string name) {
    return setSymbol(array, name, getActiveWorkspace());
}

/**
 * Add an array to a specific workspace with the specified symbol name.
 *
 * Example:
 *
 * Given _myWorkspace_ is a GEWorkspace object
 *
 * #### Python ####
 * ~~~{.py}
orders = [2, 2, 2]
data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]
a = GEArray(orders, data)
ge.setSymbol(a, "a", myWorkspace)
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$orders = array(2, 2, 2);
$data = array(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
$a = new GEArray($orders, $data);
$ge->setSymbol($a, "a", $myWorkspace);
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
int[] orders = new int[] { 2, 2, 2 };
double[] data = new double[] { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0 };
GEArray a = new GEArray(orders, data);
ge.setSymbol(a, "a", myWorkspace);
 * ~~~-->
 *
 * @param array        Array object to store in GAUSS symbol table
 * @param name        Name to give newly added symbol
 * @return True on success, false on failure
 *
 * @see setSymbol(GEArray*, string)
 * @see getArray(string)
 * @see getArrayAndClear(string)
 */
bool GAUSS::setSymbol(GEArray *array, string name, GEWorkspace *wh) {
    if (!array || name.empty())
        return false;

    if (!manager_->isValidWorkspace(wh))
        return false;

    Array_t *newArray = createPermArray(array);

    if (!newArray)
        return false;

    return (GAUSS_MoveArrayToGlobal(wh->workspace(), newArray, removeConst(&name)) == GAUSS_SUCCESS);
}

/**
 * Add a string to the active workspace with the specified symbol name.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
s = GEString("Hello World")
ge.setSymbol(s, "s")
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$s = new GEString("Hello World");
$ge->setSymbol($s, "s");
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
GEString s = new GEString("Hello World");
ge.setSymbol(s, "s");
 * ~~~-->
 *
 * @param str        string to add to GAUSS symbol table
 * @param name        Name to give newly added symbol
 * @return True on success, false on failure
 *
 * @see setSymbol(GEString*, string, GEWorkspace*)
 * @see getString(string)
 */
bool GAUSS::setSymbol(GEString *str, string name) {
    return setSymbol(str, name, getActiveWorkspace());
}

/**
 * Add a string to a specific workspace with the specified symbol name.
 *
 * Example:
 *
 * Given _myWorkspace_ is a GEWorkspace object
 *
 * #### Python ####
 * ~~~{.py}
s = GEString("Hello World")
ge.setSymbol(s, "s", myWorkspace)
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$s = new GEString("Hello World");
$ge->setSymbol($s, "s", $myWorkspace);
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
GEString s = new GEString("Hello World");
ge.setSymbol(s, "s", myWorkspace);
 * ~~~-->
 *
 * @param str        string to add to GAUSS symbol table
 * @param name        Name to give newly added symbol
 * @return True on success, false on failure
 *
 * @see setSymbol(GEString*, string)
 * @see getString(string)
 */
bool GAUSS::setSymbol(GEString *str, string name, GEWorkspace *wh) {
    if (!str || name.empty())
        return false;

    if (!manager_->isValidWorkspace(wh))
        return false;

    String_t* newStr = createPermString(str);

    if (!newStr)
        return false;

    return (GAUSS_MoveStringToGlobal(wh->workspace(), newStr, removeConst(&name)) == GAUSS_SUCCESS);
}

/**
 * Add a string array to the active workspace with the specified symbol name.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
saData = ["one", "two", "three", "four"]
sa = GEStringArray(saData, 2, 2)
ge.setSymbol(sa, "sa")
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$saData = array("one", "two", "three", "four");
$sa = new GEStringArray(saData, 2, 2);
$ge->setSymbol($sa, "sa");
 * ~~~
 *
 * @param sa        string array to add to GAUSS symbol table
 * @param name        Name to give newly added symbol
 * @return True on success, false on failure
 *
 * @see getStringArray(string)
 */
bool GAUSS::setSymbol(GEStringArray *sa, string name) {
    return setSymbol(sa, name, getActiveWorkspace());
}

/**
 * Add a string array to a specific workspace with the specified symbol name.
 *
 * Example:
 *
 * Given _myWorkspace_ is a GEWorkspace object
 *
 * #### Python ####
 * ~~~{.py}
saData = ["one", "two", "three", "four"]
sa = GEStringArray(saData, 2, 2)
ge.setSymbol(sa, "sa", myWorkspace)
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$saData = array("one", "two", "three", "four");
$sa = new GEStringArray(saData, 2, 2);
$ge->setSymbol($sa, "sa", $myWorkspace);
 * ~~~
 *
 * @param sa        string array to add to GAUSS symbol table
 * @param name        Name to give newly added symbol
 * @return True on success, false on failure
 *
 * @see getStringArray(string)
 */
bool GAUSS::setSymbol(GEStringArray *sa, string name, GEWorkspace *wh) {
    if (!sa || name.empty())
        return false;

    if (!manager_->isValidWorkspace(wh))
        return false;

    StringArray_t *newSa = createTempStringArray(sa);

    if (!newSa)
        return false;

    bool ret = (GAUSS_CopyStringArrayToGlobal(wh->workspace(), newSa, removeConst(&name)) == GAUSS_SUCCESS);

    GAUSS_Free(newSa->table);
    GAUSS_Free(newSa);

    return ret;
}

/**
 * Translates a file that contains a dataloop, so it can be read by the compiler.
 * After translating a file, you can compile it with compileFile(string) and then
 * run it with executeProgram(ProgramHandle_t*).
 *
 * If you want to see any errors that translateDataloopFile encounters,
 * then you must call setProgramErrorOutput(IGEProgramOutput*) before calling
 * translateDataloopFile.
 *
 * @param srcfile        Name of source file.
 * @return        Name of translated file. Empty if failure.
 *
 * @see compileFile(string)
 * @see setProgramErrorOutput(IGEProgramOutput*)
 *
 */
string GAUSS::translateDataloopFile(string srcfile) {
    char transbuf[1024];

    int ret = GAUSS_TranslateDataloopFile(transbuf, removeConst(&srcfile));

    if (ret != GAUSS_SUCCESS)
        return string();

    return string(transbuf);
}

Matrix_t* GAUSS::createTempMatrix(GEMatrix *mat) {
    Matrix_t *newMat = new Matrix_t;

    // THIS CANNOT BE USED FOR A "MOVE" OPERATION
    // THIS IS THE ACTUAL POINTER REFERENCE TO THE DATA.
    newMat->mdata = mat->data_;
    newMat->rows = mat->getRows();
    newMat->cols = mat->getCols();
    newMat->complex = mat->isComplex();
    newMat->freeable = 0;

    return newMat;
}

String_t* GAUSS::createPermString(GEString *str) {
    String_t *newStr = GAUSS_MallocString_t();

    string data = str->getData();
    newStr->stdata = (char*)GAUSS_Malloc(data.size() + 1);
    strncpy(newStr->stdata, data.c_str(), data.size() + 1);
    newStr->length = data.size() + 1;
    newStr->freeable = 1;

    return newStr;
}

StringArray_t* GAUSS::createTempStringArray(GEStringArray *sa) {
    vector<string> *strings = &(sa->data_);

    char **saList = new char*[sa->size()];

    for (int i = 0; i < sa->size(); ++i) {
        string str = strings->at(i);

        char *str_ptr = new char[str.length() + 1];
        strncpy(str_ptr, str.c_str(), str.length() + 1);
        saList[i] = str_ptr;
    }

    StringArray_t *newSa = GAUSS_StringArray(sa->getRows(), sa->getCols(), saList);

    for (int i = 0; i < sa->size(); ++i) {
        delete[] saList[i];
    }

    delete[] saList;

    return newSa;
}

Array_t* GAUSS::createPermArray(GEArray *array) {
    vector<double> vals = array->getData();
    vector<int> orders = array->getOrders();

    const int dims = array->getDimensions();
    const int size = array->size();

    double *data = (double*)GAUSS_Malloc((size + dims) * sizeof(double));

    for (int i = 0; i < dims; ++i)
        data[i] = orders[i];

    for (int i = dims; i < size + dims; ++i)
        data[i] = vals[i - dims];

    Array_t *newArray = (Array_t*)GAUSS_Malloc(sizeof(Array_t));
    newArray->dims = dims;
    newArray->nelems = size;
    newArray->complex = static_cast<int>(array->isComplex());
    newArray->adata = data;
    newArray->freeable = 1;

    return newArray;
}

void GAUSS::clearOutput() {
    pthread_mutex_lock(&GAUSS::kOutputMutex);
    int tid = getThreadId();
    GAUSS::kOutputStore[tid] = string();
    pthread_mutex_unlock(&GAUSS::kOutputMutex);
}

void GAUSS::clearErrorOutput() {
    pthread_mutex_lock(&GAUSS::kErrorMutex);
    int tid = getThreadId();
    GAUSS::kErrorStore[tid] = string();
    pthread_mutex_unlock(&GAUSS::kErrorMutex);
}

string GAUSS::getOutput() {
    pthread_mutex_lock(&GAUSS::kOutputMutex);

    int tid = getThreadId();

    string ret = GAUSS::kOutputStore[tid];

    GAUSS::kOutputStore[tid] = string();

    pthread_mutex_unlock(&GAUSS::kOutputMutex);

	return ret;
}

string GAUSS::getErrorOutput() {
    pthread_mutex_lock(&GAUSS::kErrorMutex);

    int tid = getThreadId();

    string ret = GAUSS::kErrorStore[tid];

    GAUSS::kErrorStore[tid] = string();

    pthread_mutex_unlock(&GAUSS::kErrorMutex);

    return ret;
}

void GAUSS::internalHookOutput(char *output) {
    pthread_mutex_lock(&GAUSS::kOutputMutex);

    int tid = getThreadId();

    string store = GAUSS::kOutputStore[getThreadId()];

    store.append(output);

    GAUSS::kOutputStore[tid] = store;

    pthread_mutex_unlock(&GAUSS::kOutputMutex);
}

void GAUSS::internalHookError(char *output) {
    pthread_mutex_lock(&GAUSS::kErrorMutex);

    int tid = getThreadId();

    string store = GAUSS::kErrorStore[getThreadId()];

    store.append(output);

    GAUSS::kErrorStore[tid] = store;

    pthread_mutex_unlock(&GAUSS::kErrorMutex);
}

void GAUSS::internalHookFlush() {
    if (GAUSS::flushFunc_) {
        GAUSS::flushFunc_->invoke();
    } else {
        cout.flush();
        cerr.flush();
    }
}

int GAUSS::internalHookInputString(char *buf, int len) {
    memset(buf, 0, len);

    // Check for user input string function.
    if (GAUSS::inputStringFunc_) {
        GAUSS::inputStringFunc_->clear();
        GAUSS::inputStringFunc_->invoke(len);

        string ret = GAUSS::inputStringFunc_->value();

        // write ret data to buf;
        strncpy(buf, ret.c_str(), len);
        return ret.length();
    }

    return 0;
}

int GAUSS::internalHookInputChar() {
    if (GAUSS::inputCharFunc_) {
        return GAUSS::inputCharFunc_->invoke();
    }

    return -1;
}

int GAUSS::internalHookInputBlockingChar() {
    if (GAUSS::inputBlockingCharFunc_) {
        return GAUSS::inputBlockingCharFunc_->invoke();
    }

    return -1;
}

int GAUSS::internalHookInputCheck() {
    if (GAUSS::inputCheckFunc_) {
        return GAUSS::inputCheckFunc_->invoke();
    }

    return 0;
}

/**
 * Dually set the program output hook as well as the error output hook. Program output is any time the
 * GAUSS Engine needs to display any information back to the user.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
class Output(IGEProgramOutput):
    def invoke(self, message):
        print message,

out = Output()
out.thisown = 0
ge.setProgramOutputAll(out)
ge.executeString("rndseed 12345")
ge.executeString("rndu(3, 3)")  # Will produce valid output
ge.executeString("y")           # Will produce error: y does not exist
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
class Output extends IGEProgramOutput {
    function invoke($message) {
        echo $message;
    }
}

$out = new Output();
$out->thisown = 0;

$ge->setProgramOutputAll($out);
$ge->executeString("rndseed 12345");
$ge->executeString("rndu(3, 3)"); // Will produce valid output
$ge->executeString("y"); // Will produce error: y does not exist
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
// The object that implements a callback function MUST stay in scope and not be garbage-collected
// for the entire time that you want to use that particular callback function. If it is prematurely
// garbage-collected, it could cause the JVM to crash.
GEProgramOutput outputFn = new GEProgramOutput() {
    public void invoke(String message) {
        System.out.print(message);
    }
};

ge.setProgramOutput(outputFn);
ge.executeString("rndseed 12345");
ge.executeString("rndu(3, 3)"); // Will produce valid output
ge.executeString("y"); // Will produce error: y does not exist
 * ~~~-->
 *
 * will result in the output:
 * ~~~
      0.90483859        0.44540096        0.76257185
      0.12938459        0.50966033       0.062034276
      0.70726755       0.077567409        0.83558391

Undefined symbols:
    y
 * ~~~
 *
 * @param fn        User-defined output function.
 *
 * @see setProgramOutput(IGEProgramOutput*)
 * @see setProgramErrorOutput(IGEProgramOutput*)
 */
void GAUSS::setProgramOutputAll(IGEProgramOutput *func) {
    GAUSS::outputFunc_ = func;
    GAUSS::errorFunc_ = func;
}

/**
 * Sets the program output hook. Program output is any time the
 * GAUSS Engine needs to display any information back to the user.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
class Output(IGEProgramOutput):
    def invoke(self, message):
        print message,

out = Output()
out.thisown = 0

ge.setProgramOutputAll(out)
ge.executeString("rndseed 12345")
ge.executeString("rndu(3, 3)")  # Will produce valid output
ge.executeString("y")           # Will produce error: y does not exist
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
class Output extends IGEProgramOutput {
    function invoke($message) {
        echo $message;
    }
}

$out = new Output();
$out->thisown = 0;

$ge->setProgramOutput($out);
$ge->executeString("rndseed 12345");
$ge->executeString("rndu(3, 3)");
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
// The object that implements a callback function MUST stay in scope and not be garbage-collected
// for the entire time that you want to use that particular callback function. If it is prematurely
// garbage-collected, it could cause the JVM to crash.
GEProgramOutput outputFn = new GEProgramOutput() {
    public void invoke(String message) {
        System.out.print(message);
    }
};

ge.setProgramOutput(outputFn);
ge.executeString("rndseed 12345");
ge.executeString("rndu(3, 3)");
 * ~~~-->
 *
 * will result in the output:
 * ~~~
      0.90483859        0.44540096        0.76257185
      0.12938459        0.50966033       0.062034276
      0.70726755       0.077567409        0.83558391
 * ~~~
 *
 * @param fn        User-defined output function.
 *
 * @see setProgramOutputAll(IGEProgramOutput*)
 * @see setProgramErrorOutput(IGEProgramOutput*)
 */
void GAUSS::setProgramOutput(IGEProgramOutput *func) {
    GAUSS::outputFunc_ = func;
}

/**
 * Sets the program error output hook. Program error output is any time the
 * GAUSS Engine needs to display any error information back to the user.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
class Output(IGEProgramOutput):
    def invoke(self, message):
        print message,

out = Output()
out.thisown = 0

ge.setProgramErrorOutput(out)
ge.executeString("y")           # Will produce error: y does not exist
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
class Output extends IGEProgramOutput {
    function invoke($message) {
        echo $message;
    }
}

$out = new Output();
$out->thisown = 0;

$ge->setProgramErrorOutput($out);
$ge->executeString("y");        // Will produce error: y does not exist
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
// The object that implements a callback function MUST stay in scope and not be garbage-collected
// for the entire time that you want to use that particular callback function. If it is prematurely
// garbage-collected, it could cause the JVM to crash.
GEProgramOutput outputFn = new GEProgramOutput() {
    public void invoke(String message) {
        System.out.print("Error: " + message);
    }
};

ge.setProgramOutput(outputFn);
ge.executeString("y"); // Will produce error: y does not exist
 * ~~~-->
 *
 * will result in the output:
 * ~~~
Undefined symbols:
    y
 * ~~~
 *
 * @param fn        User-defined output function.
 *
 * @see setProgramOutputAll(IGEProgramOutput*)
 * @see setProgramOutput(IGEProgramOutput*)
 */
void GAUSS::setProgramErrorOutput(IGEProgramOutput *func) {
    GAUSS::errorFunc_ = func;
}

/**
 * Forces flushing of buffered output.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
class FlushOutput(IGEProgramFlushOutput):
    def invoke(self):
        print "A flush was requested."

flush = FlushOutput()
flush.thisown = 0

ge.setProgramFlushOutput(flush)
ge.executeString("print /flush \"Hello World!\"")
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
class FlushOutput extends IGEProgramFlushOutput {
    function invoke() {
        echo "A flush was requested.";
    }
}

$flush = new FlushOutput();
$flush->thisown = 0;

$ge->setProgramFlushOutput($flush);
$ge->executeString("print /flush \"Hello World!\"");
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
// The object that implements a callback function MUST stay in scope and not be garbage-collected
// for the entire time that you want to use that particular callback function. If it is prematurely
// garbage-collected, it could cause the JVM to crash.
IGEProgramFlushOutput flushFn = new IGEProgramFlushOutput() {
    public void invoke() {
        System.out.println("A flush was requested.");
    }
};

ge.setProgramFlushOutput(flushFn);
ge.executeString("print /flush \"Hello World!\"");
 * ~~~-->
 *
 * will result in the output:
 * ~~~
Hello World!
A flush was requested.
 * ~~~
 *
 * @param fn        User-defined flush output function.
 *
 * @see setProgramOutput(IGEProgramOutput*)
 * @see setProgramErrorOutput(IGEProgramOutput*)
 */
void GAUSS::setProgramFlushOutput(IGEProgramFlushOutput *func) {
    GAUSS::flushFunc_ = func;
}

/**
 * Set the callback function that GAUSS will call for blocking string input. This function should block
 * until a user-supplied string of input is available.
 *
 * #### GAUSS commands which activate this callback ####
 * - `cons`
 *
 * #### Python ####
 * ~~~{.py}
class StringInput(IGEProgramInputString):
    # The callback does not return a string directly, rather through a method call.
    def invoke(self, length) {
        self.setValue("Hello World!")

strCallback = StringInput();
strCallback.thisown = 0;

ge.setProgramInputString(strCallback)
ge.executeString("s = cons")

s = ge.getString("s")
print "s = " + s.getData()
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
class StringInput extends IGEProgramInputString {
    // The callback does not return a string directly, rather through a method call.
    function invoke($length) {
        $this->setValue("Hello World!");
    }
}

$strCallback = new StringInput();
$strCallback->thisown = 0;

$ge->setProgramInputString($strCallback);
$ge->executeString("s = cons");

$s = $ge->getString("s");
echo "s = " . $s->getData() . PHP_EOL;
 * ~~~
 *
 * <!--#### Java <NOT IMPLEMENTED> ####
 * ~~~{.java}
// The object that implements a callback function MUST stay in scope and not be garbage-collected
// for the entire time that you want to use that particular callback function. If it is prematurely
// garbage-collected, it could cause the JVM to crash.
GEProgramInputString consFn = new GEProgramInputString() {
    @Override
    public void invoke(int len) {
        // Ask user for input normally
        this.setValue("Hello World!");
    }
};

ge.setProgramInputString(consFn);
ge.executeString("s = cons");
GEString s = ge.getString("s");
System.out.println("s = " + s.getData());
 * ~~~-->
 *
 * will result in the output:
 * ~~~
 * s = Hello World!
 * ~~~
 *
 * @param fn        User-defined output function.
 *
 * @see setProgramInputChar(IGEProgramInputChar*)
 * @see setProgramInputCharBlocking(IGEProgramInputChar*)
 * @see setProgramInputCheck(IGEProgramInputCheck*)
 */
void GAUSS::setProgramInputString(IGEProgramInputString *func) {
    GAUSS::inputStringFunc_ = func;
}

/**
 * Indicate the callback function that a GAUSS program should call for (non-blocking) single character input.
 *
 * This function should return immediately, with either the value of a user-supplied character
 * or a 0 if none is available.
 *
 * #### GAUSS commands which activate this callback ####
 * - `key`
 *
 * #### Python ####
 * ~~~{.py}
class CharInput(IGEProgramInputChar):
    def invoke(self):
        # Return buffered input
        return ord("c") # Return the integer value of 'c'

charCallback = CharInput()
charCallback.thisown = 0

ge.setProgramInputChar(charCallback)
ge.executeString("k = key")

k = ge.getScalar("k")
print "k = " + str(k)
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
class CharInput extends IGEProgramInputChar {
    function invoke() {
        // Return buffered input
        return ord("c"); // Return the integer value of 'c'
    }
}

$charCallback = new CharInput();
$charCallback->thisown = 0;

$ge->setProgramInputChar($charCallback);
$ge->executeString("k = key");

$k = $ge->getScalar("k");
echo "k = " . $k . PHP_EOL;
 * ~~~
 *
 * <!--#### Java <NOT IMPLEMENTED> ####
 * ~~~{.java}
// The object that implements a callback function MUST stay in scope and not be garbage-collected
// for the entire time that you want to use that particular callback function. If it is prematurely
// garbage-collected, it could cause the JVM to crash.
IGEProgramInputChar charFn = new IGEProgramInputChar {
    @Override
    public void invoke() {
        // Return buffered input
        return Character.getNumericValue('c'); // Return the integer value of 'c'
    }
}

ge.setProgramInputChar(charFn);
ge.executeString("k = key");

double k = ge.getScalar("k");
System.out.println("k = " + k);
 * ~~~-->
 *
 * will result in the output:
 * ~~~
k = 99
 * ~~~
 *
 * @param fn        User-defined input function.
 *
 * @see setProgramInputString(IGEProgramInputString*)
 * @see setProgramInputCharBlocking(IGEProgramInputChar*)
 * @see setProgramInputCheck(IGEProgramInputCheck*)
 */
void GAUSS::setProgramInputChar(IGEProgramInputChar *func) {
    GAUSS::inputCharFunc_ = func;
}

/**
 * Indicate the callback function that a GAUSS program should call for blocking single character input.
 * This function should block
 * until user-supplied input is available, then return the value of a single character.
 *
 * #### GAUSS commands which activate this callback ####
 * - `keyw`
 *
 * #### Python ####
 * ~~~{.py}
class CharInput(IGEProgramInputChar):
    def invoke(self):
        # Return buffered input
        return ord("c") # Return the integer value of 'c'

charCallback = CharInput()
charCallback.thisown = 0

ge.setProgramInputCharBlocking(charCallback)
ge.executeString("k = key")

k = ge.getScalar("k")
print "k = " + str(k)
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
class CharInput extends IGEProgramInputChar {
    function invoke() {
        // Block for user input
        return ord("c"); // Return the integer value of 'c'
    }
}

$charCallback = new CharInput();
$charCallback->thisown = 0;

$ge->setProgramInputCharBlocking($charCallback);
$ge->executeString("k = keyw");

$k = $ge->getScalar("k");
echo "k = " . $k . PHP_EOL;
 * ~~~
 *
 * <!--#### Java <NOT IMPLEMENTED> ####
 * ~~~{.java}
// The object that implements a callback function MUST stay in scope and not be garbage-collected
// for the entire time that you want to use that particular callback function. If it is prematurely
// garbage-collected, it could cause the JVM to crash.
IGEProgramInputChar charFn = new IGEProgramInputChar {
    @Override
    public void invoke() {
        // Block for user input
        return Character.getNumericValue('c'); // Return the integer value of 'c'
    }
}

ge.setProgramInputCharBlocking(charFn);
ge.executeString("k = keyw");

double k = ge.getScalar("k");
System.out.println("k = " + k);
 * ~~~-->
 *
 * will result in the output:
 * ~~~
k = 99
 * ~~~
 *
 * @param fn        User-defined input function.
 *
 * @see setProgramInputString(IGEProgramInputString*)
 * @see setProgramInputChar(IGEProgramInputChar*)
 * @see setProgramInputCheck(IGEProgramInputCheck*)
 */
void GAUSS::setProgramInputCharBlocking(IGEProgramInputChar *func) {
    GAUSS::inputBlockingCharFunc_ = func;
}

/**
 * Indicate the callback function that a GAUSS program should call to see if user-supplied input is available.
 * This function should return 1 if there is pending input available, 0 if not.
 *
 * #### GAUSS commands which activate this callback ####
 * - `keyav`
 *
 * #### Python ####
 * ~~~{.py}
class InputCheck(IGEProgramInputCheck):
    def invoke(self):
        # We pretend we have character input available.
        print "Input check requested"
        return 1

inputCheckCallback = InputCheck()
inputCheckCallback.thisown = 0

ge.setProgramInputCheck(inputCheckCallback)
ge.executeString("av = keyav")
ge.executeString("if (av); print \"Key available\"; endif")
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
class InputCheck extends IGEProgramInputCheck {
    function invoke() {
        // We pretend we have character input available.
        echo "Input check requested" . PHP_EOL;
        return 1;
    }
}

$inputCheckCallback = new InputCheck();
$inputCheckCallback->thisown = 0;

$ge->setProgramInputCheck($inputCheckCallback);
$ge->executeString("av = keyav");
$ge->executeString("if (av); print \"Key available\"; endif");
 * ~~~
 * results in output:
 * ~~~
Input check requested
Key available
 * ~~~
 *
 * @param fn        User-defined input function.
 *
 * @see setProgramInputString(IGEProgramInputString*)
 * @see setProgramInputChar(IGEProgramInputChar*)
 * @see setProgramInputCharBlocking(IGEProgramInputChar*)
 */
void GAUSS::setProgramInputCheck(IGEProgramInputCheck *func) {
    GAUSS::inputCheckFunc_ = func;
}

void GAUSS::setHookOutput(void (*display_string_function)(char *str)) {
    setHookProgramOutput(display_string_function);
    setHookProgramErrorOutput(display_string_function);
}

void GAUSS::setHookProgramErrorOutput(void (*display_error_string_function)(char *)) {
    GAUSS_HookProgramErrorOutput(display_error_string_function);
}

void GAUSS::setHookProgramOutput(void (*display_string_function)(char *str)) {
    GAUSS_HookProgramOutput(display_string_function);
}

void GAUSS::setHookFlushProgramOutput(void (*flush_display_function)(void)) {
    GAUSS_HookFlushProgramOutput(flush_display_function);
}

void GAUSS::setHookProgramInputChar(int (*get_char_function)(void)) {
    GAUSS_HookProgramInputChar(get_char_function);
}

void GAUSS::setHookProgramInputBlockingChar(int (*get_char_blocking_function)(void)) {
    GAUSS_HookProgramInputCharBlocking(get_char_blocking_function);
}

void GAUSS::setHookProgramInputString(int (*get_string_function)(char *, int)) {
    GAUSS_HookProgramInputString(get_string_function);
}

void GAUSS::setHookProgramInputCheck(int (*get_string_function)(void)) {
    GAUSS_HookProgramInputCheck(get_string_function);
}

//void GAUSS::setHookGetCursorPosition(int (*get_cursor_position_function)(void)) {
//    GAUSS_HookGetCursorPosition(get_cursor_position_function);
//}

GAUSS::~GAUSS() {
    if (this->manager_)
        delete this->manager_;

    if (GAUSS::outputFunc_) {
        // Prevent double-delete for user doing setProgramOutputAll
        if (GAUSS::outputFunc_ == GAUSS::errorFunc_)
            GAUSS::errorFunc_ = 0;

        delete GAUSS::outputFunc_;
        GAUSS::outputFunc_ = 0;
    }

    if (GAUSS::errorFunc_) {
        delete GAUSS::errorFunc_;
        GAUSS::errorFunc_ = 0;
    }

    if (GAUSS::flushFunc_) {
        delete GAUSS::flushFunc_;
        GAUSS::flushFunc_ = 0;
    }

    if (GAUSS::inputStringFunc_) {
        delete GAUSS::inputStringFunc_;
        GAUSS::inputStringFunc_ = 0;
    }

    if (GAUSS::inputCharFunc_) {
        if (GAUSS::inputCharFunc_ == GAUSS::inputBlockingCharFunc_)
            GAUSS::inputBlockingCharFunc_ = 0;

        delete GAUSS::inputCharFunc_;
        GAUSS::inputCharFunc_ = 0;
    }

    if (GAUSS::inputBlockingCharFunc_) {
        delete GAUSS::inputBlockingCharFunc_;
        GAUSS::inputBlockingCharFunc_ = 0;
    }

    if (GAUSS::inputCheckFunc_) {
        delete GAUSS::inputCheckFunc_;
        GAUSS::inputCheckFunc_ = 0;
    }
}
