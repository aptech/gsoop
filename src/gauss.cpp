#include <cstring>
#include <cctype>    // for isalnum()
#include <algorithm> // for back_inserter
#include <iostream>

#include "gearray.h"
#include "gematrix.h"
#include "gestringarray.h"
#include "geworkspace.h"
#include "workspacemanager.h"
#include "gefuncwrapper.h"
#include "gauss_p.h"
#include "gauss.h"

#ifdef _WIN32
#include "windows.h"
#else
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#endif

#include <stdio.h>
#include <mutex>
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
static double kMissingValue = GAUSS_MissingValue();
static std::string kHomeVar = "MTENGHOME";

static unordered_map<int, std::string> kOutputStore;
static std::mutex kOutputMutex;
static unordered_map<int, std::string> kErrorStore;
static std::mutex kErrorMutex;

IGEProgramOutput* GAUSS::outputFunc_ = 0;
IGEProgramOutput* GAUSS::errorFunc_ = 0;
IGEProgramFlushOutput* GAUSS::flushFunc_ = 0;
IGEProgramInputString* GAUSS::inputStringFunc_ = 0;
IGEProgramInputChar* GAUSS::inputCharFunc_ = 0;
IGEProgramInputChar* GAUSS::inputBlockingCharFunc_ = 0;
IGEProgramInputCheck* GAUSS::inputCheckFunc_ = 0;

static char* removeConst(std::string *str) {
    return const_cast<char*>(str->c_str());
}

static int getThreadId() {
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
 * Initialize the library using the environment variable value of `MTENGHOME` as the
 * path for the GAUSS Home path.
 *
 * @see GAUSS(std::string, bool)
 */
GAUSS::GAUSS()
{
    char *envVal = getenv(kHomeVar.c_str());

    std::string homeVal;

    if (envVal)
        homeVal = std::string(envVal);

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
GAUSS::GAUSS(std::string inp, bool isEnvVar) {
    std::string homeVal = inp;

    if (isEnvVar) {
        char *envVal = getenv(inp.c_str());

        if (envVal)
            homeVal = std::string(envVal);
    }

    Init(homeVal);
}

void GAUSS::Init(std::string homePath) {
    this->d = new GAUSSPrivate(homePath);

    resetHooks();

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
    if (!setHome(this->d->gauss_home_)) {
        std::string errorString = getLastErrorText();

        cerr << "Could not set GAUSS Home (Error: " << errorString << ")" << endl;
		cerr.flush();
        return false;
    }

    if (GAUSS_Initialize() >  0) {
        std::string errorString = getLastErrorText();

        cerr << "Could initialize GAUSS (Error: " << errorString << ")" << endl;
		cerr.flush();
        return false;
    }

    GEWorkspace *wh = createWorkspace("main");

    if (wh->workspace() == NULL) {
        std::string errorString = getLastErrorText();

        cerr << "Could not create workspace (Error: " << errorString << ")" << endl;
		cerr.flush();
        return false;
    }
    
    setActiveWorkspace(wh);

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
GEWorkspace* GAUSS::createWorkspace(std::string name) {
    return this->d->manager_->create(name);
}

/**
 * Free a workspace. Deletes all associated symbols. This cannot be undone.
 *
 * Note that you will not be able to manipulate symbols without an active workspace.
 *
 * @param wh Workspace handle
 * @return Whether workspace was successfully removed
 *
 * @see createWorkspace(std::string)
 * @see destroyAllWorkspaces()
 */
bool GAUSS::destroyWorkspace(GEWorkspace *wh) {
    return this->d->manager_->destroy(wh);
}

/**
 * Clears all workspaces. Note that you will not be able to manipulate symbols
 * without an active workspace.
 *
 * @see createWorkspace(std::string)
 * @see destroyWorkspace(GEWorkspace*)
 */
void GAUSS::destroyAllWorkspaces() {
    this->d->manager_->destroyAll();
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
GEWorkspace* GAUSS::getWorkspace(std::string name) const {
    return this->d->manager_->getWorkspace(name);
}

/**
 * Return a handle to the currently active workspace.
 *
 * @return Active workspace objectut
 *
 * @see setActiveWorkspace(GEWorkspace*)
 */
GEWorkspace* GAUSS::getActiveWorkspace() const {
    return this->d->manager_->getCurrent();
}

/**
 * Saves workspace information contained in a workspace handle into a file.
 * The file will have the name given by _fn_. Load the workspace information with loadWorkspace(std::string).
 *
 * @param wh        Workspace object
 * @param fn        Filename to save workspace as
 *
 * @see loadWorkspace(std::string)
 */
bool GAUSS::saveWorkspace(GEWorkspace *wh, std::string fn) {
    if (!this->d->manager_->isValidWorkspace(wh))
        return false;

    return (GAUSS_SaveWorkspace(wh->workspace(), removeConst(&fn)) == GAUSS_SUCCESS);
}

/**
 * Saves a compiled program given by a program handle into a file. It saves all of the
 * workspace information, which is contained in the program handle. The file will have
 * the name given by _fn_. Load the program with loadCompiledFile(std::string).
 *
 * @param ph        Program handle
 * @param fn        Filename to save program to
 * @return        True on success, false on failure
 *
 * @see loadCompiledFile(std::string)
 */
bool GAUSS::saveProgram(ProgramHandle_t *ph, std::string fn) {
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
    return this->d->manager_->setCurrent(wh);
}

/**
 * Returns the current path known by the GAUSS Engine for the user home directory.
 *
 * @return        Path to user GAUSS home directory.
 *
 * @see setHome(std::string)
 */
std::string GAUSS::getHome() const {
    char buf[1024];

    GAUSS_GetHome(buf);

    return std::string(buf);
}

/**
 * Returns the current environment variable value set by the GAUSS Engine that represents the GAUSS Home path.
 *
 * @return        Environment variable name
 *
 * @see setHomeVar(std::string)
 */
std::string GAUSS::getHomeVar() const {
    char buf[1024];

    GAUSS_GetHomeVar(buf);

    return std::string(buf);
}

/**
 * Returns the current log file location used by the GAUSS Engine
 *
 * @return        Path to log file
 *
 * @see setLogFile(std::string, std::string)
 */
std::string GAUSS::getLogFile() const {
    char buf[1024];

    GAUSS_GetLogFile(buf);

    return std::string(buf);
}

/**
 * Turns a given path into an absolute path.
 *
 * @param path     Path to be analyzed
 * @return      Absolute representation of _path_ argument
 */
std::string GAUSS::makePathAbsolute(std::string path) {
    char buf[4096];

    memset(buf, 0, sizeof(buf));

    strncpy(buf, path.c_str(), sizeof(buf));

    GAUSS_MakePathAbsolute(buf);

    return std::string(buf);
}

/**
 * Calls the program input std::string function hooked
 * with setProgramInputString(IGEProgramInputString*).
 *
 * The callbacks are thread specific. programInputString will call the
 * input std::string function that was hooked in that particular thread.
 *
 * @return        user input from hooked function
 *
 * @see setProgramInputString(IGEProgramInputString*)
 */
std::string GAUSS::programInputString() {
    char buf[4096];

    GAUSS_ProgramInputString(buf, 4096);

    return std::string(buf);
}

/**
 * Executes a command within the GAUSS Engine on the currently active workspace. If you wish to
 * run this command repeatedly, you can compile it first using compileString(std::string) and
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
 * @see compileString(std::string)
 */
bool GAUSS::executeString(std::string command) {
    return executeString(command, getActiveWorkspace());
}

/**
 * Executes a command within the GAUSS Engine on a specific workspace. If you wish to
 * run this command repeatedly, you can compile it first using compileString(std::string) and
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
 * @see compileString(std::string)
 */
bool GAUSS::executeString(std::string command, GEWorkspace *wh) {
    if (!this->d->manager_->isValidWorkspace(wh))
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
 * run this file repeatedly, you can compile it first using compileFile(std::string) and
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
$success = $ge->executeFile("ols.e");
 * ~~~
 *
 * @param filename        Filename to execute.
 * @return true on success; false on failure
 *
 * @see compileFile(std::string)
 */
bool GAUSS::executeFile(std::string fname) {
    return executeFile(fname, getActiveWorkspace());
}

/**
 * Executes a named file within the GAUSS Engine on the a specific workspace. If you wish to
 * run this file repeatedly, you can compile it first using compileFile(std::string) and
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
$success = $ge->executeFile("ols.e", $myWorkspace);
 * ~~~
 *
 * @param filename        Filename to execute.
 * @return true on success; false on failure
 *
 * @see compileFile(std::string)
 */
bool GAUSS::executeFile(std::string fname, GEWorkspace *wh) {
    if (!this->d->manager_->isValidWorkspace(wh))
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
 * was called. If you wish to run this file repeatedly, you can load it first using loadCompiledFile(std::string)
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
 * @see loadWorkspace(std::string)
 * @see loadCompiledFile(std::string)
 */
bool GAUSS::executeCompiledFile(std::string fname) {
    return executeCompiledFile(fname, getActiveWorkspace());
}

/**
 * Executes a compiled gcg file within the GAUSS Engine on a specific workspace. As soon as
 * the file is finished executing it sets the current workspace to what it was before this function
 * was called. If you wish to run this file repeatedly, you can load it first using loadCompiledFile(std::string)
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
 * @see loadWorkspace(std::string)
 * @see loadCompiledFile(std::string)
 */
bool GAUSS::executeCompiledFile(std::string fname, GEWorkspace *wh) {
    if (!this->d->manager_->isValidWorkspace(wh))
        return false;

    ProgramHandle_t *ph = GAUSS_LoadCompiledFile(wh->workspace(), removeConst(&fname));

    if (!ph)
        return false;

    bool ret = executeProgram(ph);

    GAUSS_FreeProgram(ph);

    return ret;
}

/**
 * Compiles a std::string and returns a program handle in the active workspace. This can then be followed with a call
 * to executeProgram(ProgramHandle_t*). Note that if you do not care about keeping the program handle,
 * a convenience method executeString(std::string) is available.
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
 * @see executeString(std::string)
 * @see freeProgram
 */
ProgramHandle_t* GAUSS::compileString(std::string command) {
    return compileString(command, getActiveWorkspace());
}

/**
 * Compiles a std::string and returns a program handle in the specified workspace. This can then be followed with a call
 * to executeProgram(ProgramHandle_t*). Note that if you do not care about keeping the program handle,
 * a convenience method executeString(std::string) is available.
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
 * @see executeString(std::string)
 * @see freeProgram
 */
ProgramHandle_t* GAUSS::compileString(std::string command, GEWorkspace *wh) {
    if (!this->d->manager_->isValidWorkspace(wh))
        return NULL;

    return GAUSS_CompileString(wh->workspace(), removeConst(&command), 0, 0);
}

/**
 * Compiles a file and returns a program handle in the active workspace. This can then be followed with a call
 * to executeProgram(ProgramHandle_t*). Note that if you do not care about keeping the program handle,
 * a convenience method executeFile(std::string) is available.
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
 * @see executeFile(std::string)
 * @see freeProgram
 */
ProgramHandle_t* GAUSS::compileFile(std::string fname) {
    return compileFile(fname, getActiveWorkspace());
}

/**
 * Compiles a file and returns a program handle in a specific workspace. This can then be followed with a call
 * to executeProgram(ProgramHandle_t*). Note that if you do not care about keeping the program handle,
 * a convenience method executeFile(std::string) is available.
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
 * @see executeFile(std::string)
 */
ProgramHandle_t* GAUSS::compileFile(std::string fname, GEWorkspace *wh) {
    if (!this->d->manager_->isValidWorkspace(wh))
        return NULL;

    return GAUSS_CompileFile(wh->workspace(), removeConst(&fname), 0, 0);
}

/**
 * Loads an already compiled file into the active workspace and returns a program handle. This can then
 * be followed with a call to executeProgram(ProgramHandle_t*). Note that if you do not care about
 * keeping the program handle, a convenience method executeCompiledFile(std::string) is available.
 *
 * @param fn        Filename to load
 * @return        Program handle
 *
 * @see executeProgram(ProgramHandle_t*)
 * @see executeCompiledFile(std::string)
 */
ProgramHandle_t* GAUSS::loadCompiledFile(std::string fn) {
    return loadCompiledFile(fn, getActiveWorkspace());
}

/**
 * Loads an already compiled file into a specific workspace and returns a program handle. This can then
 * be followed with a call executeProgram(ProgramHandle_t*). Note that if you do not care about
 * keeping the program handle, a convenience method executeCompiledFile(std::string) is available.
 *
 * @param fn        Filename to load
 * @return        Program handle
 *
 * @see executeProgram(ProgramHandle_t*)
 * @see executeCompiledFile(std::string)
 */
ProgramHandle_t* GAUSS::loadCompiledFile(std::string fn, GEWorkspace *wh) {
    if (!this->d->manager_->isValidWorkspace(wh))
        return NULL;

    return GAUSS_LoadCompiledFile(wh->workspace(), removeConst(&fn));
}

/**
 * Executes a given program handle that was created with either compileString(std::string), compileFile(std::string),
 * or loadCompiledFile(std::string).
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
 * @see compileString(std::string)
 * @see compileFile(std::string)
 * @see loadCompiledFile(std::string)
 * @see freeProgram(ProgramHandle_t*)
 */
bool GAUSS::executeProgram(ProgramHandle_t *ph) {
    if (!ph)
        return false;

    // Setup output hook
    resetHooks();

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
GEWorkspace* GAUSS::loadWorkspace(std::string gcgfile) {
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
std::string GAUSS::getWorkspaceName(GEWorkspace *wh) const {
    if (!this->d->manager_->isValidWorkspace(wh))
        return std::string();

    char name[1024];
    memset(&name, 0, sizeof(name));

    GAUSS_GetWorkspaceName(wh->workspace(), name);

    return std::string(name);
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
    std::string wkspName = getWorkspaceName(wh);

    wh->setName(wkspName);
}

/**
 * Free a program handle created by compileString(std::string),
 * compileFile(std::string), and loadCompiledFile(std::string)
 *
 * @param ph        Program handle
 *
 * @see compileString(std::string)
 * @see compileFile(std::string)
 * @see loadCompiledFile(std::string)
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
 * ~~~{.py}
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
int GAUSS::getSymbolType(std::string name) const {
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
int GAUSS::getSymbolType(std::string name, GEWorkspace *wh) const {
    if (!this->d->manager_->isValidWorkspace(wh))
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

/**
 * Allows you to set the file that the errors will be logged in.
 * The Engine logs certain system level errors in 2 places: a
 * file and an open file pointer. The default file is `/tmp/mteng.###.log` where
 * `###` is the process ID number. The default file pointer is stderr.
 *
 * You can turn off the error logging to file by inputting an empty std::string for _logfn_.
 *
 * @param logfn        name of log file.
 * @param mode        **w** to overwrite the contents of the file.\n **a** to append to the contents of the file.
 * @return        True on success, false on failure
 *
 * @see getLogFile()
 */
bool GAUSS::setLogFile(std::string logfn, std::string mode) {
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
 * @see setHomeVar(std::string)
 */
bool GAUSS::setHome(std::string path) {
    return GAUSS_SetHome(removeConst(&path)) == GAUSS_SUCCESS;
}

/**
 * The default value is `MTENGHOME`.
 *
 * It is better to use setHome(std::string) which sets the home directory, overriding
 * the environment variable.
 *
 * @param envVar        Name of environment variable
 * @return        True on success, false on failure
 *
 * @see getHomeVar()
 * @see setHome(std::string)
 */
bool GAUSS::setHomeVar(std::string envVar) {
    return GAUSS_SetHomeVar(removeConst(&envVar)) == GAUSS_SUCCESS;
}

/**
 * Return the std::string error description of the last error code.
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
std::string GAUSS::getLastErrorText() const {
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
int GAUSS::getError() const {
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
std::string GAUSS::getErrorText(int errorNum) const {
    char buf[1024];

    GAUSS_ErrorText(buf, errorNum);

    return std::string(buf);
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

bool GAUSS::_setSymbol(GESymbol *symbol, std::string name) {
    return _setSymbol(symbol, name, getActiveWorkspace());
}

bool GAUSS::_setSymbol(GESymbol *symbol, std::string name, GEWorkspace *wh) {
    if (!symbol || name.empty() || !this->d->manager_->isValidWorkspace(wh))
        return false;

    switch(symbol->type()) {
    case GESymType::SCALAR:
    case GESymType::MATRIX:
        return setSymbol(static_cast<GEMatrix*>(symbol), name, wh);
        break;
    case GESymType::ARRAY_GAUSS:
        return setSymbol(static_cast<GEArray*>(symbol), name, wh);
        break;
    case GESymType::STRING:
    case GESymType::STRING_ARRAY:
        return setSymbol(static_cast<GEStringArray*>(symbol), name, wh);
        break;
    default:
        return false;
    }
}

GESymbol* GAUSS::getSymbol(std::string name) const {
    return getSymbol(name, getActiveWorkspace());
}

GESymbol* GAUSS::getSymbol(std::string name, GEWorkspace *wh) const {
    if (name.empty() || !this->d->manager_->isValidWorkspace(wh))
        return 0;

    int type = getSymbolType(name, wh);

    switch (type) {
    case GESymType::SCALAR:
    case GESymType::MATRIX:
        return getMatrix(name, wh);
        break;
    case GESymType::ARRAY_GAUSS:
        return getArray(name, wh);
        break;
    case GESymType::STRING:
    case GESymType::STRING_ARRAY:
        return getStringArray(name, wh);
        break;
    default:
        return nullptr;
    }
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
 * @see getScalar(std::string, GEWorkspace*)
 * @see setSymbol(GEMatrix*, std::string)
 * @see setSymbol(GEMatrix*, std::string, GEWorkspace*)
 * @see getMatrixAndClear(std::string)
 * @see getMatrixAndClear(std::string, GEWorkspace*)
 */
double GAUSS::getScalar(std::string name) const {
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
 * @see getScalar(std::string)
 * @see setSymbol(GEMatrix*, std::string)
 * @see setSymbol(GEMatrix*, std::string, GEWorkspace*)
 * @see getMatrixAndClear(std::string)
 * @see getMatrixAndClear(std::string, GEWorkspace*)
 */
double GAUSS::getScalar(std::string name, GEWorkspace *wh) const {
    if (!this->d->manager_->isValidWorkspace(wh))
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
 * first calling setSymbol(GEMatrix*, std::string).
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
 * @see getMatrix(std::string, GEWorkspace*)
 * @see setSymbol(GEMatrix*, std::string)
 * @see setSymbol(GEMatrix*, std::string, GEWorkspace*)
 * @see getMatrixAndClear(std::string)
 * @see getMatrixAndClear(std::string, GEWorkspace*)
 * @see getScalar(std::string)
 * @see getScalar(std::string, GEWorkspace*)
 */
GEMatrix* GAUSS::getMatrix(std::string name) const {
    return getMatrix(name, getActiveWorkspace());
}

/**
 * Retrieve a matrix from the GAUSS symbol name in workspace _wh_. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without
 * first calling setSymbol(GEMatrix*, std::string).
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
 * @see getMatrix(std::string)
 * @see setSymbol(GEMatrix*, std::string)
 * @see setSymbol(GEMatrix*, std::string, GEWorkspace*)
 * @see getMatrixAndClear(std::string)
 * @see getMatrixAndClear(std::string, GEWorkspace*)
 * @see getScalar(std::string)
 * @see getScalar(std::string, GEWorkspace*)
 */
GEMatrix* GAUSS::getMatrix(std::string name, GEWorkspace *wh) const {
    if (!this->d->manager_->isValidWorkspace(wh))
        return NULL;

	GAUSS_MatrixInfo_t *info = new GAUSS_MatrixInfo_t;
	int ret = GAUSS_GetMatrixInfo(wh->workspace(), info, removeConst(&name));

	if (ret) {
		delete info;
		return NULL;
	}

    return new GEMatrix(info);
}

/**
 * Retrieve a matrix from the GAUSS symbol name in the active workspace. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without
 * first calling setSymbol(GEMatrix*, std::string).
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
 * @see getMatrixAndClear(std::string, GEWorkspace*)
 * @see setSymbol(GEMatrix*, std::string)
 * @see setSymbol(GEMatrix*, std::string, GEWorkspace*)
 * @see getMatrix(std::string)
 * @see getMatrix(std::string, GEWorkspace*)
 * @see getScalar(std::string)
 * @see getScalar(std::string, GEWorkspace*)
 */
GEMatrix* GAUSS::getMatrixAndClear(std::string name) const {
    return getMatrixAndClear(name, getActiveWorkspace());
}

/**
 * Retrieve a matrix from the GAUSS symbol name in workspace _wh_. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without
 * first calling setSymbol(GEMatrix*, std::string).
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
 * @see getMatrixAndClear(std::string)
 * @see setSymbol(GEMatrix*, std::string)
 * @see setSymbol(GEMatrix*, std::string, GEWorkspace*)
 * @see getMatrix(std::string)
 * @see getMatrix(std::string, GEWorkspace*)
 * @see getScalar(std::string)
 * @see getScalar(std::string, GEWorkspace*)
 */
GEMatrix* GAUSS::getMatrixAndClear(std::string name, GEWorkspace *wh) const {
    if (!this->d->manager_->isValidWorkspace(wh))
        return NULL;

    Matrix_t *gsMat = GAUSS_GetMatrixAndClear(wh->workspace(), removeConst(&name));

    if (gsMat == NULL)
        return NULL;

    return new GEMatrix(gsMat);
}

/**
*
* NOTICE: This function is intended for advanced usage only. It provides a direct pointer to the data
*     inside the GAUSS symbol table. There are no guarantees or bounds checking performed when accessing
*     memory provided by this function. Do so at your own risk.
*
* Retrieve pointer to a matrix from the GAUSS symbol name in the active workspace. This will be the ORIGINAL symbol in the symbol table.
*
* Example:
*
* Given _myWorkspace_ is a GEWorkspace object
*
* #### Python ####
* ~~~{.py}
ge.executeString("x = 5")
x = ge.getMatrixDirect("x")
print "\$x = " + str(x.getitem(0))
ge.executeString("print \"x = \" x");
* ~~~
*
* #### PHP ####
* ~~~{.php}
$ge->executeString("x = 5");
$x = $ge->getMatrixDirect("x");
echo "\$x = " . $x->getitem(0) . PHP_EOL;
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
* @see getMatrixDirect(std::string, GEWorkspace*)
* @see setSymbol(GEMatrix*, std::string)
* @see setSymbol(GEMatrix*, std::string, GEWorkspace*)
* @see getMatrix(std::string)
* @see getMatrix(std::string, GEWorkspace*)
* @see getScalar(std::string)
* @see getScalar(std::string, GEWorkspace*)
*/
doubleArray* GAUSS::getMatrixDirect(std::string name) {
	return getMatrixDirect(name, getActiveWorkspace());
}

/**
*
* NOTICE: This function is intended for advanced usage only. It provides a direct pointer to the data
*     inside the GAUSS symbol table. There are no guarantees or bounds checking performed when accessing
*     memory provided by this function. Do so at your own risk.
*
* Retrieve pointer to a matrix from the GAUSS symbol name in workspace _wh_. This will be the ORIGINAL symbol in the symbol table.
*
* Example:
*
* Given _myWorkspace_ is a GEWorkspace object
*
* #### Python ####
* ~~~{.py}
ge.executeString("x = 5", myWorkspace)
x = ge.getMatrixDirect("x", myWorkspace)
print "\$x = " + str(x.getitem(0))
ge.executeString("print \"x = \" x", myWorkspace);
* ~~~
*
* #### PHP ####
* ~~~{.php}
$ge->executeString("x = 5", $myWorkspace);
$x = $ge->getMatrixDirect("x", $myWorkspace);
echo "\$x = " . $x->getitem(0) . PHP_EOL;
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
* @see getMatrixDirect(std::string)
* @see setSymbol(GEMatrix*, std::string)
* @see setSymbol(GEMatrix*, std::string, GEWorkspace*)
* @see getMatrix(std::string)
* @see getMatrix(std::string, GEWorkspace*)
* @see getScalar(std::string)
* @see getScalar(std::string, GEWorkspace*)
*/
doubleArray* GAUSS::getMatrixDirect(std::string name, GEWorkspace* wh) {
    if (name.empty() || !this->d->manager_->isValidWorkspace(wh))
		return NULL;

	GAUSS_MatrixInfo_t info;
	int ret = GAUSS_GetMatrixInfo(wh->workspace(), &info, removeConst(&name));

    if (ret)
		return NULL;

    return new doubleArray(info.maddr, info.rows * info.cols);
}

bool GAUSS::_setSymbol(doubleArray *data, std::string name) {
    return _setSymbol(data, name, getActiveWorkspace());
}

bool GAUSS::_setSymbol(doubleArray *data, std::string name, GEWorkspace *wh) {
    if (!data || name.empty() || !this->d->manager_->isValidWorkspace(wh))
        return false;

    return moveMatrix(data, 1, data->size(), false, name, wh);
}

/**
 * Retrieve an array from the GAUSS symbol table in the active workspace. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without first
 * calling setSymbol(GEArray*, std::string).
 *
 * @param name        Name of GAUSS symbol
 * @return        Array object
 *
 * @see getArray(std::string, GEWorkspace*)
 * @see setSymbol(GEArray*, std::string)
 * @see setSymbol(GEArray*, std::string, GEWorkspace*)
 * @see getArrayAndClear(std::string)
 * @see getArrayAndClear(std::string, GEWorkspace*)
 */
GEArray* GAUSS::getArray(std::string name) const {
    return getArray(name, getActiveWorkspace());
}

/**
 * Retrieve an array from the GAUSS symbol table in workspace _wh_. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without first
 * calling setSymbol(GEArray*, std::string).
 *
 * @param name        Name of GAUSS symbol
 * @return        Array object
 *
 * @see getArray(std::string)
 * @see setSymbol(GEArray*, std::string)
 * @see setSymbol(GEArray*, std::string, GEWorkspace*)
 * @see getArrayAndClear(std::string)
 * @see getArrayAndClear(std::string, GEWorkspace*)
 */
GEArray* GAUSS::getArray(std::string name, GEWorkspace *wh) const {
    if (!this->d->manager_->isValidWorkspace(wh))
        return NULL;

    Array_t *gsArray = GAUSS_GetArray(wh->workspace(), removeConst(&name));

    if (gsArray == NULL)
        return NULL;

    return new GEArray(gsArray);
}

/**
 * Retrieve an array from the GAUSS symbol table in the active workspace. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without first
 * calling setSymbol(GEArray*, std::string).
 *
 * In addition, this function will clear the symbol from the GAUSS symbol table.
 *
 * @param name        Name of GAUSS symbol
 * @return        Array object
 *
 * @see getArrayAndClear(std::string, GEWorkspace*)
 * @see setSymbol(GEArray*, std::string)
 * @see setSymbol(GEArray*, std::string, GEWorkspace*)
 * @see getArray(std::string)
 * @see getArray(std::string, GEWorkspace*)
 */
GEArray* GAUSS::getArrayAndClear(std::string name) const {
    return getArrayAndClear(name, getActiveWorkspace());
}

/**
 * Retrieve an array from the GAUSS symbol table in workspace _wh_. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without first
 * calling setSymbol(GEArray*, std::string).
 *
 * In addition, this function will clear the symbol from the GAUSS symbol table.
 *
 * @param name        Name of GAUSS symbol
 * @return        Array object
 *
 * @see getArrayAndClear(std::string)
 * @see setSymbol(GEArray*, std::string)
 * @see setSymbol(GEArray*, std::string, GEWorkspace*)
 * @see getArray(std::string)
 * @see getArray(std::string, GEWorkspace*)
 */
GEArray* GAUSS::getArrayAndClear(std::string name, GEWorkspace *wh) const {
    if (!this->d->manager_->isValidWorkspace(wh))
        return NULL;

    Array_t *gsArray = GAUSS_GetArrayAndClear(wh->workspace(), removeConst(&name));

    if (gsArray == NULL)
        return NULL;

    return new GEArray(gsArray);
}

/**
 * Retrieve a string array from the GAUSS symbol table in the active workspace. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without first
 * calling setSymbol(GEStringArray*, std::string).
 *
 * @param name        Name of GAUSS symbol
 * @return        string array object
 *
 * @see getStringArray(std::string, GEWorkspace*)
 * @see setSymbol(GEStringArray*, std::string)
 * @see setSymbol(GEStringArray*, std::string, GEWorkspace*)
 */
GEStringArray* GAUSS::getStringArray(std::string name) const {
    return getStringArray(name, getActiveWorkspace());
}

/**
 * Retrieve a string array from the GAUSS symbol table in workspace _wh_. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without first
 * calling setSymbol(GEStringArray*, std::string).
 *
 * @param name        Name of GAUSS symbol
 * @return        string array object
 *
 * @see getStringArray(std::string)
 * @see setSymbol(GEStringArray*, std::string)
 * @see setSymbol(GEStringArray*, std::string, GEWorkspace*)
 */
GEStringArray* GAUSS::getStringArray(std::string name, GEWorkspace *wh) const {
    if (!this->d->manager_->isValidWorkspace(wh))
        return NULL;

    StringArray_t *gsStringArray = GAUSS_GetStringArray(wh->workspace(), removeConst(&name));

    if (gsStringArray == NULL)
        return NULL;

    return new GEStringArray(gsStringArray);
}

/**
 * Retrieve a std::string from the GAUSS symbol table in the active workspace. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without
 * first calling setSymbol(std::string, name).
 *
 * @param name    Name of GAUSS symbol
 * @return        std::string object
 *
 * @see getString(std::string, GEWorkspace*)
 * @see setSymbol(std::string, std::string)
 * @see setSymbol(std::string, std::string, GEWorkspace*)
 */
std::string GAUSS::getString(std::string name) const {
    return getString(name, getActiveWorkspace());
}

/**
 * Retrieve a std::string from the GAUSS symbol table in workspace _wh_. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without
 * first calling setSymbol(std::string, std::string).
 *
 * @param name    Name of GAUSS symbol
 * @return        std::string object
 *
 * @see getString(std::string)
 * @see setSymbol(std::string, std::string)
 * @see setSymbol(std::string, std::string, GEWorkspace*)
 */
std::string GAUSS::getString(std::string name, GEWorkspace *wh) const {
    std::string ret;
    if (!this->d->manager_->isValidWorkspace(wh))
        return ret;

    String_t *gsString = GAUSS_GetString(wh->workspace(), removeConst(&name));

    if (gsString == NULL || gsString->stdata == NULL)
        return ret;

    ret = std::string(gsString->stdata);
    GAUSS_Free(gsString->stdata);
    GAUSS_Free(gsString);


    return ret;
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
 * @see setSymbol(GEMatrix*, std::string, GEWorkspace*)
 * @see getMatrix(std::string)
 * @see getMatrixAndClear(std::string)
 * @see getScalar(std::string)
 */
bool GAUSS::setSymbol(GEMatrix *matrix, std::string name) {
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
 * @see setSymbol(GEMatrix*, std::string)
 * @see getMatrix(std::string)
 * @see getMatrixAndClear(std::string)
 * @see getScalar(std::string)
 */
bool GAUSS::setSymbol(GEMatrix *matrix, std::string name, GEWorkspace *wh) {
    if (!matrix || name.empty())
        return false;

    if (!this->d->manager_->isValidWorkspace(wh))
        return false;

    int ret = 0;

    if (!matrix->isComplex() && (matrix->getRows() == 1) && (matrix->getCols() == 1)) {
        ret = GAUSS_PutDouble(wh->workspace(), matrix->getElement(), removeConst(&name));
    } else {
        Matrix_t* newMat = this->d->createTempMatrix(matrix);

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
 * @see setSymbol(GEArray*, std::string, GEWorkspace*)
 * @see getArray(std::string)
 * @see getArrayAndClear(std::string)
 */
bool GAUSS::setSymbol(GEArray *array, std::string name) {
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
 * @see setSymbol(GEArray*, std::string)
 * @see getArray(std::string)
 * @see getArrayAndClear(std::string)
 */
bool GAUSS::setSymbol(GEArray *array, std::string name, GEWorkspace *wh) {
    if (!array || name.empty())
        return false;

    if (!this->d->manager_->isValidWorkspace(wh))
        return false;

    Array_t *newArray = this->d->createTempArray(array);

    if (!newArray)
        return false;

    return (GAUSS_CopyArrayToGlobal(wh->workspace(), newArray, removeConst(&name)) == GAUSS_SUCCESS);
}

/**
 * Add a std::string to the active workspace with the specified symbol name.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
s = "Hello World"
ge.setSymbol(s, "s")
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$s = "Hello World";
$ge->setSymbol($s, "s");
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
s = "Hello World";
ge.setSymbol(s, "s");
 * ~~~-->
 *
 * @param str        std::string to add to GAUSS symbol table
 * @param name        Name to give newly added symbol
 * @return True on success, false on failure
 *
 * @see setSymbol(std::string, std::string, GEWorkspace*)
 * @see getString(std::string)
 */
bool GAUSS::setSymbol(std::string str, std::string name) {
    return setSymbol(str, name, getActiveWorkspace());
}

/**
 * Add a std::string to a specific workspace with the specified symbol name.
 *
 * Example:
 *
 * Given _myWorkspace_ is a GEWorkspace object
 *
 * #### Python ####
 * ~~~{.py}
s = "Hello World"
ge.setSymbol(s, "s", myWorkspace)
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$s = "Hello World";
$ge->setSymbol($s, "s", $myWorkspace);
 * ~~~
 *
 * <!--#### Java ####
 * ~~~{.java}
s = "Hello World";
ge.setSymbol(s, "s", myWorkspace);
 * ~~~-->
 *
 * @param str        std::string to add to GAUSS symbol table
 * @param name        Name to give newly added symbol
 * @return True on success, false on failure
 *
 * @see setSymbol(std::string, std::string)
 * @see getString(std::string)
 */
bool GAUSS::setSymbol(std::string str, std::string name, GEWorkspace *wh) {
    if (name.empty())
        return false;

    if (!this->d->manager_->isValidWorkspace(wh))
        return false;

    String_t* newStr = this->d->createPermString(str);

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
 * @see getStringArray(std::string)
 */
bool GAUSS::setSymbol(GEStringArray *sa, std::string name) {
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
 * @see getStringArray(std::string)
 */
bool GAUSS::setSymbol(GEStringArray *sa, std::string name, GEWorkspace *wh) {
    if (!sa || name.empty())
        return false;

    if (!this->d->manager_->isValidWorkspace(wh))
        return false;

    StringArray_t *newSa = this->d->createPermStringArray(sa);

    if (!newSa)
        return false;

    bool ret = (GAUSS_MoveStringArrayToGlobal(wh->workspace(), newSa, removeConst(&name)) == GAUSS_SUCCESS);

    //GAUSS_Free(newSa->table);
    //GAUSS_Free(newSa);

    return ret;
}

/**
* Add a matrix to the active workspace with the specified symbol name.
* This implementation clears the local data after the move is completed.
*
* Example:
*
* #### Python ####
* ~~~{.py}
x = GEMatrix(5.0)
ge.moveSymbol(x, "x")
* ~~~
*
* #### PHP ####
* ~~~{.php}
$x = new GEMatrix(5);
$ge->moveSymbol($x, "x");
* ~~~
*
* <!--#### Java ####
* ~~~{.java}
GEMatrix x = new GEMatrix(5);
ge.moveSymbol(x, "x");
* ~~~->
*
* @param matrix    Matrix object to store in GAUSS symbol table
* @param name      Name to give newly added symbol
* @return          True on success, false on failure
*
* @see moveSymbol(GEMatrix*, std::string, GEWorkspace*)
* @see getMatrix(std::string)
* @see getMatrixAndClear(std::string)
* @see getScalar(std::string)
*/
bool GAUSS::moveSymbol(GEMatrix *matrix, std::string name) {
	return moveSymbol(matrix, name, getActiveWorkspace());
}

/**
* Add a matrix to a specific workspace with the specified symbol name.
* This implementation clears the local data after the move is completed.
*
* Example:
*
* Given _myWorkspace_ is a GEWorkspace object
*
* #### Python ####
* ~~~{.py}
x = GEMatrix(5.0)
ge.moveSymbol(x, "x", myWorkspace)
* ~~~
*
* #### PHP ####
* ~~~{.php}
$x = new GEMatrix(5);
$ge->moveSymbol($x, "x", $myWorkspace);
* ~~~
*
* <!--#### Java ####
* ~~~{.java}
GEMatrix x = new GEMatrix(5);
ge.moveSymbol(x, "x", myWorkspace);
* ~~~-->
*
* @param matrix    Matrix object to store in GAUSS symbol table
* @param name      Name to give newly added symbol
* @return          True on success, false on failure
*
* @see moveSymbol(GEMatrix*, std::string)
* @see getMatrix(std::string)
* @see getMatrixAndClear(std::string)
* @see getScalar(std::string)
*/
bool GAUSS::moveSymbol(GEMatrix *matrix, std::string name, GEWorkspace *wh) {
	if (!matrix || name.empty())
		return false;

	if (!this->d->manager_->isValidWorkspace(wh))
		return false;

	int ret = 0;

	if (!matrix->isComplex() && (matrix->getRows() == 1) && (matrix->getCols() == 1)) {
		ret = GAUSS_PutDouble(wh->workspace(), matrix->getElement(), removeConst(&name));
	}
	else {
		Matrix_t* newMat = this->d->createTempMatrix(matrix);

		ret = GAUSS_CopyMatrixToGlobal(wh->workspace(), newMat, removeConst(&name));
		delete newMat;
	}

	matrix->clear();

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
ge.moveSymbol(a, "a")
* ~~~
*
* #### PHP ####
* ~~~{.php}
$orders = array(2, 2, 2);
$data = array(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
$a = new GEArray($orders, $data);
$ge->moveSymbol($a, "a");
* ~~~
*
* <!--#### Java ####
* ~~~{.java}
int[] orders = new int[] { 2, 2, 2 };
double[] data = new double[] { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0 };
GEArray a = new GEArray(orders, data);
ge.moveSymbol(a, "a");
* ~~~-->
*
* @param array        Array object to store in GAUSS symbol table
* @param name        Name to give newly added symbol
* @return True on success, false on failure
*
* @see moveSymbol(GEArray*, std::string, GEWorkspace*)
* @see getArray(std::string)
* @see getArrayAndClear(std::string)
*/
bool GAUSS::moveSymbol(GEArray *array, std::string name) {
	return moveSymbol(array, name, getActiveWorkspace());
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
ge.moveSymbol(a, "a", myWorkspace)
* ~~~
*
* #### PHP ####
* ~~~{.php}
$orders = array(2, 2, 2);
$data = array(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
$a = new GEArray($orders, $data);
$ge->moveSymbol($a, "a", $myWorkspace);
* ~~~
*
* <!--#### Java ####
* ~~~{.java}
int[] orders = new int[] { 2, 2, 2 };
double[] data = new double[] { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0 };
GEArray a = new GEArray(orders, data);
ge.moveSymbol(a, "a", myWorkspace);
* ~~~-->
*
* @param array        Array object to store in GAUSS symbol table
* @param name        Name to give newly added symbol
* @return True on success, false on failure
*
* @see moveSymbol(GEArray*, std::string)
* @see getArray(std::string)
* @see getArrayAndClear(std::string)
*/
bool GAUSS::moveSymbol(GEArray *array, std::string name, GEWorkspace *wh) {
	if (!array || name.empty())
		return false;

	if (!this->d->manager_->isValidWorkspace(wh))
		return false;

	Array_t *newArray = this->d->createTempArray(array);

	if (!newArray)
		return false;

	bool ret = (GAUSS_CopyArrayToGlobal(wh->workspace(), newArray, removeConst(&name)) == GAUSS_SUCCESS);

	if (ret)
		array->clear();

	return ret;
}

/**
* Add a string array to the active workspace with the specified symbol name.
* This implementation clears the local data after the move is completed.
*
* Example:
*
* #### Python ####
* ~~~{.py}
saData = ["one", "two", "three", "four"]
sa = GEStringArray(saData, 2, 2)
ge.moveSymbol(sa, "sa")
* ~~~
*
* #### PHP ####
* ~~~{.php}
$saData = array("one", "two", "three", "four");
$sa = new GEStringArray(saData, 2, 2);
$ge->moveSymbol($sa, "sa");
* ~~~
*
* @param sa        string array to add to GAUSS symbol table
* @param name        Name to give newly added symbol
* @return True on success, false on failure
*
* @see getStringArray(std::string)
*/
bool GAUSS::moveSymbol(GEStringArray *sa, std::string name) {
	return moveSymbol(sa, name, getActiveWorkspace());
}

/**
* Add a string array to a specific workspace with the specified symbol name.
* This implementation clears the local data after the move is completed.
*
* Example:
*
* Given _myWorkspace_ is a GEWorkspace object
*
* #### Python ####
* ~~~{.py}
saData = ["one", "two", "three", "four"]
sa = GEStringArray(saData, 2, 2)
ge.moveSymbol(sa, "sa", myWorkspace)
* ~~~
*
* #### PHP ####
* ~~~{.php}
$saData = array("one", "two", "three", "four");
$sa = new GEStringArray(saData, 2, 2);
$ge->moveSymbol($sa, "sa", $myWorkspace);
* ~~~
*
* @param sa        string array to add to GAUSS symbol table
* @param name        Name to give newly added symbol
* @return True on success, false on failure
*
* @see getStringArray(std::string)
*/
bool GAUSS::moveSymbol(GEStringArray *sa, std::string name, GEWorkspace *wh) {
	if (!sa || name.empty())
		return false;

	if (!this->d->manager_->isValidWorkspace(wh))
		return false;

	StringArray_t *newSa = this->d->createPermStringArray(sa);

	if (!newSa)
		return false;

	bool ret = (GAUSS_MoveStringArrayToGlobal(wh->workspace(), newSa, removeConst(&name)) == GAUSS_SUCCESS);

	sa->clear();

	return ret;
}

/**
* Add a matrix and give ownership to the active workspace with the specified symbol name.
* 
* WARNING: This is a low-level function. Once this function has been called it
* is potentially unsafe to access the object passed in  if GAUSS has 
* performed any operation that is not "in-place" (ie the memory has moved).
*
*
* Example:
*
* #### Python ####
* ~~~{.py}
x = doubleArray(1)
x.setitem(0, 5.0);
ge.moveMatrix(x.cast(), 1, 1, False, "x")
* ~~~
*
* #### PHP ####
* ~~~{.php}
$x = new doubleArray(1);
$x->setitem(0, 5.0);
$ge->moveMatrix($x->cast(), 1, 1, false, "x");
* ~~~
*
* <!--#### Java ####
* ~~~{.java}
doubleArray x = new doubleArray(1);
x.setitem(0, 5.0);
ge.moveMatrix(x.cast(), 1, 1, false, "x");
* ~~~->
*
* @param data      doubleArray wrapper containing data to assign to symbol table
* @param rows      Row count
* @param cols      Column count
* @param complex   True if data contains complex data, False otherwise
* @param name      Name to give newly added symbol
* @return          True on success, false on failure
*
* @see moveSymbol(GEMatrix*, std::string, GEWorkspace*)
* @see getMatrix(std::string)
* @see getMatrixAndClear(std::string)
* @see getScalar(std::string)
*/
bool GAUSS::moveMatrix(doubleArray *data, int rows, int cols, bool complex, std::string name) {
    return moveMatrix(data, rows, cols, complex, name, getActiveWorkspace());
}

/**
* Add a matrix and give ownership to a specific workspace with the specified symbol name.
*
* WARNING: This is a low-level function. Once this function has been called it
* is potentially unsafe to access the object passed in if GAUSS has
* performed any operation that is not "in-place" (ie the memory has moved).
*
* Example:
*
* Given _myWorkspace_ is a GEWorkspace object
*
* #### Python ####
* ~~~{.py}
x = doubleArray(2)
x.setitem(0, 5.0);
x.setitem(1, 10.0);
ge.moveMatrix(x.cast(), 2, 1, False, "x", myWorkspace)
* ~~~
*
* #### PHP ####
* ~~~{.php}
$x = new doubleArray(2);
$x->setitem(0, 5.0);
$x->setitem(1, 10.0);
$ge->moveMatrix($x->cast(), 2, 1, false, "x", $myWorkspace);
* ~~~
*
* <!--#### Java ####
* ~~~{.java}
doubleArray x = new doubleArray(2);
x.setitem(0, 5.0);
x.setitem(1, 10.0);
ge.moveMatrix(x.cast(), 2, 1, false, "x", myWorkspace);
* ~~~->
*
* @param data      doubleArray wrapper containing data to assign to symbol table
* @param rows      Row count
* @param cols      Column count
* @param complex   True if data contains complex data, False otherwise
* @param name      Name to give newly added symbol
* @param wh        Workspace to assign symbol to
* @return          True on success, false on failure
*
* @see moveMatrix(double*,int,int,bool,std::string)
* @see getMatrix(std::string)
* @see getMatrixAndClear(std::string)
*/
bool GAUSS::moveMatrix(doubleArray *data, int rows, int cols, bool complex, std::string name, GEWorkspace *wh) {
    if (!data || name.empty() || !this->d->manager_->isValidWorkspace(wh))
		return false;

    int ret = GAUSS_AssignFreeableMatrix(wh->workspace(), rows, cols, complex, data->data(), removeConst(&name));

    data->reset();

	return (ret == GAUSS_SUCCESS);
}

/**
 * Translates a file that contains a dataloop, so it can be read by the compiler.
 * After translating a file, you can compile it with compileFile(std::string) and then
 * run it with executeProgram(ProgramHandle_t*).
 *
 * If you want to see any errors that translateDataloopFile encounters,
 * then you must call setProgramErrorOutput(IGEProgramOutput*) before calling
 * translateDataloopFile.
 *
 * @param srcfile        Name of source file.
 * @return        Name of translated file. Empty if failure.
 *
 * @see compileFile(std::string)
 * @see setProgramErrorOutput(IGEProgramOutput*)
 *
 */
std::string GAUSS::translateDataloopFile(std::string srcfile) {
    char transbuf[1024];

    int ret = GAUSS_TranslateDataloopFile(transbuf, removeConst(&srcfile));

    if (ret != GAUSS_SUCCESS)
        return std::string();

    return std::string(transbuf);
}

void GAUSS::clearOutput() {
    std::lock_guard<std::mutex> guard(kOutputMutex);
    int tid = getThreadId();
    kOutputStore[tid] = std::string();
}

void GAUSS::clearErrorOutput() {
    std::lock_guard<std::mutex> guard(kErrorMutex);
    int tid = getThreadId();
    kErrorStore[tid] = std::string();
}

std::string GAUSS::getOutput() {
    if (!GAUSS::outputModeManaged())
        return std::string();

    std::lock_guard<std::mutex> guard(kOutputMutex);
    int tid = getThreadId();

    std::string ret = kOutputStore[tid];
    kOutputStore[tid] = std::string();

	return ret;
}

std::string GAUSS::getErrorOutput() {
    if (!GAUSS::outputModeManaged())
        return std::string();

    std::lock_guard<std::mutex> guard(kErrorMutex);

    int tid = getThreadId();

    std::string ret = kErrorStore[tid];
    kErrorStore[tid] = std::string();

    return ret;
}

void GAUSS::resetHooks() {
    setHookProgramOutput(GAUSS::internalHookOutput);
    setHookProgramErrorOutput(GAUSS::internalHookError);
    setHookFlushProgramOutput(GAUSS::internalHookFlush);
    setHookProgramInputString(GAUSS::internalHookInputString);
    setHookProgramInputChar(GAUSS::internalHookInputChar);
    setHookProgramInputBlockingChar(GAUSS::internalHookInputBlockingChar);
    setHookProgramInputCheck(GAUSS::internalHookInputCheck);
}

void GAUSS::internalHookOutput(char *output) {
    if (GAUSS::outputModeManaged()) {
        std::lock_guard<std::mutex> guard(kOutputMutex);
        int tid = getThreadId();
        std::string &store = kOutputStore[tid];
        store.append(output);
    } else if (GAUSS::outputFunc_) {
        GAUSS::outputFunc_->invoke(std::string(output));
    } else {
        fprintf(stdout, output);
    }
}

void GAUSS::internalHookError(char *output) {
    if (GAUSS::outputModeManaged()) {
        std::lock_guard<std::mutex> guard(kErrorMutex);
        int tid = getThreadId();
        std::string &store = kErrorStore[tid];
        store.append(output);
    } else if (GAUSS::errorFunc_) {
        GAUSS::errorFunc_->invoke(std::string(output));
    } else {
        fprintf(stderr, output);
    }
}

void GAUSS::internalHookFlush() {
    if (GAUSS::flushFunc_) {
        GAUSS::flushFunc_->invoke();
    } else {
        fflush(stdout);
        fflush(stderr);
    }
}

int GAUSS::internalHookInputString(char *buf, int len) {
    memset(buf, 0, len);

    // Check for user input std::string function.
    if (GAUSS::inputStringFunc_) {
        GAUSS::inputStringFunc_->clear();
        GAUSS::inputStringFunc_->invoke(len);

        std::string ret = GAUSS::inputStringFunc_->value();

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

void GAUSS::setOutputModeManaged(bool managed) {
    GAUSSPrivate::managedOutput_ = managed;
}

bool GAUSS::outputModeManaged() {
    return GAUSSPrivate::managedOutput_;
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
 * Set the callback function that GAUSS will call for blocking std::string input. This function should block
 * until a user-supplied std::string of input is available.
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
print "s = " + s
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
echo "s = " . $s . PHP_EOL;
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
String s = ge.getString("s");
System.out.println("s = " + s);
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
//    if (GAUSS::outputFunc_) {
//        // Prevent double-delete for user doing setProgramOutputAll
//        if (GAUSS::outputFunc_ == GAUSS::errorFunc_)
//            GAUSS::errorFunc_ = 0;

//        delete GAUSS::outputFunc_;
//        GAUSS::outputFunc_ = 0;
//    }

//    if (GAUSS::errorFunc_) {
//        delete GAUSS::errorFunc_;
//        GAUSS::errorFunc_ = 0;
//    }

//    if (GAUSS::flushFunc_) {
//        delete GAUSS::flushFunc_;
//        GAUSS::flushFunc_ = 0;
//    }

//    if (GAUSS::inputStringFunc_) {
//        delete GAUSS::inputStringFunc_;
//        GAUSS::inputStringFunc_ = 0;
//    }

//    if (GAUSS::inputCharFunc_) {
//        if (GAUSS::inputCharFunc_ == GAUSS::inputBlockingCharFunc_)
//            GAUSS::inputBlockingCharFunc_ = 0;

//        delete GAUSS::inputCharFunc_;
//        GAUSS::inputCharFunc_ = 0;
//    }

//    if (GAUSS::inputBlockingCharFunc_) {
//        delete GAUSS::inputBlockingCharFunc_;
//        GAUSS::inputBlockingCharFunc_ = 0;
//    }

//    if (GAUSS::inputCheckFunc_) {
//        delete GAUSS::inputCheckFunc_;
//        GAUSS::inputCheckFunc_ = 0;
//    }
}

bool GAUSSPrivate::managedOutput_ = true;

GAUSSPrivate::GAUSSPrivate(const std::string &homePath) {
    this->gauss_home_ = homePath;
    this->manager_ = new WorkspaceManager;
}

GAUSSPrivate::~GAUSSPrivate() {
    delete this->manager_;
}

Matrix_t* GAUSSPrivate::createTempMatrix(GEMatrix *mat) {
    Matrix_t *newMat = new Matrix_t;
    // THIS CANNOT BE USED FOR A "MOVE" OPERATION
    // THIS IS THE ACTUAL POINTER REFERENCE TO THE DATA.
    newMat->mdata = mat->data_.data();
    newMat->rows = mat->getRows();
    newMat->cols = mat->getCols();
    newMat->complex = mat->isComplex();
    newMat->freeable = 0;

    return newMat;
}

String_t* GAUSSPrivate::createPermString(std::string data) {
    String_t *newStr = GAUSS_MallocString_t();

    newStr->stdata = (char*)GAUSS_Malloc(data.size() + 1);
    strncpy(newStr->stdata, data.c_str(), data.size() + 1);
    newStr->length = data.size() + 1;
    newStr->freeable = 1;

    return newStr;
}

#define getsize(R,C,S) ((R)*(C)*(S)/(size_t)8 + ( ((R)*(C)*(S))%(size_t)8 != (size_t)0 ) )

StringArray_t* GAUSSPrivate::createPermStringArray(GEStringArray *gesa) {
    if (!gesa || gesa->size() == 0)
        return NULL;

    std::vector<std::string> *strings = &(gesa->data_);

    StringArray_t *sa;
    StringElement_t *stable;
    StringElement_t *sep;
    StringElement_t *pdata;
    size_t elem;
    size_t strsize, sasize;

    sa = (StringArray_t *)malloc(sizeof(StringArray_t));

    if (sa == NULL)
        return NULL;

    elem = gesa->size();
    sa->baseoffset = (size_t)(elem*sizeof(StringElement_t));

    stable = (StringElement_t *)malloc(sa->baseoffset);

    if (stable == NULL)
    {
        free(sa);
        return NULL;
    }

    sep = stable;
    strsize = 0;

    for (int i = 0; i < elem; ++i) {
        const char *src = strings->at(i).c_str();
        sep->offset = strsize;
        sep->length = strlen(src) + 1;
        strsize += sep->length;
        ++sep;
    }

    sasize = getsize(strsize + sa->baseoffset, 1, 1);
    pdata = (StringElement_t*)realloc(stable, sasize * sizeof(double));

    if (pdata == NULL)
    {
        free(sa);
        free(stable);
        return NULL;
    }

    stable = (StringElement_t *)pdata;
    sep = stable;

    for (int i = 0; i < elem; ++i) {
        const char *src = strings->at(i).c_str();
        memcpy((char *)stable + sa->baseoffset + sep->offset, src, sep->length);
        ++sep;
    }

    sa->size = sasize;
    sa->rows = gesa->getRows();
    sa->cols = gesa->getCols();
    sa->table = stable;
    sa->freeable = TRUE;

    return sa;
}

Array_t* GAUSSPrivate::createTempArray(GEArray *array) {
    const int dims = array->getDimensions();
    const int size = array->size();

    if (!array || !dims || !size)
        return NULL;

	/*
    const double *array_data = array->data_.data();

    double *data = (double*)GAUSS_Malloc((size + dims) * sizeof(double));

    if (!data)
        return NULL;

    // copy orders
    memcpy(data, array_data, dims * sizeof(double));

    // copy remaining data (start + orders len)
    memcpy(data + dims, array_data + dims, size * sizeof(double));
	*/
    Array_t *newArray = (Array_t*)GAUSS_Malloc(sizeof(Array_t));

    if (!newArray) {
        return NULL;
    }

    newArray->dims = dims;
    newArray->nelems = size;
    newArray->complex = static_cast<int>(array->isComplex());
    newArray->adata = array->data_.data();
    newArray->freeable = TRUE;

    return newArray;
}
