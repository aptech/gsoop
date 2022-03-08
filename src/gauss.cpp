#include <cstring>
#include <cctype>    // for isalnum()
#include <algorithm> // for back_inserter
#include <iostream>
#include <memory>

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
static std::string kHomeVar = "MTENGHOME";

thread_local std::string kOutputStore;
thread_local std::string kErrorStore;

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

static bool endsWithCaseInsensitive(const std::string &mainStr, const std::string &toMatch)
{
    auto it = toMatch.begin();
    return mainStr.size() >= toMatch.size() &&
            std::all_of(std::next(mainStr.begin(), mainStr.size() - toMatch.size()), mainStr.end(), [&it](const char &c){
                return ::tolower(c) == ::tolower(*(it++));
    });
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
__Python__
```py
# Using a custom environment variable
ge = GAUSS("MY_CUSTOM_VAR");

# Setting a specific path
ge = GAUSS("/home/user/mteng", False);
```
 *
__PHP__
```php
// Using a custom environment variable
$ge = new GAUSS("MY_CUSTOM_VAR");

// Setting a specific path
$ge = new GAUSS("/home/user/mteng", false);
```
 *
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
        return false;
    }

    if (GAUSS_Initialize() >  0) {
        return false;
    }

    GEWorkspace *workspace = createWorkspace("main");

    if (workspace == nullptr || workspace->workspace() == nullptr) {
        return false;
    }

    setActiveWorkspace(workspace);

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
 * @param name    Name of workspace to create
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
 * @param workspace Workspace handle
 * @return Whether workspace was successfully removed
 *
 * @see createWorkspace(std::string)
 * @see destroyAllWorkspaces()
 */
bool GAUSS::destroyWorkspace(GEWorkspace *workspace) {
    return this->d->manager_->destroy(workspace);
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
 * @param name    Name of workspace
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
 * The file will have the name given by _filename_. Load the workspace information with loadWorkspace(std::string).
 *
 * @param workspace        Workspace object
 * @param filename        Filename to save workspace as
 *
 * @see loadWorkspace(std::string)
 */
bool GAUSS::saveWorkspace(GEWorkspace *workspace, std::string filename) {
    if (!this->d->manager_->isValidWorkspace(workspace))
        return false;

    return (GAUSS_SaveWorkspace(workspace->workspace(), removeConst(&filename)) == GAUSS_SUCCESS);
}

/**
 * Saves a compiled program given by a program handle into a file. It saves all of the
 * workspace information, which is contained in the program handle. The file will have
 * the name given by _filename. Load the program with loadCompiledFile(std::string).
 *
 * @param ph        Program handle
 * @param filename        Filename to save program to
 * @return        True on success, false on failure
 *
 * @see loadCompiledFile(std::string)
 */
bool GAUSS::saveProgram(ProgramHandle_t *ph, std::string filename) {
    return (GAUSS_SaveProgram(ph, removeConst(&filename)) == GAUSS_SUCCESS);
}

/**
 * Sets the active workspace to be the specified workspace.
 *
 * @param workspace        Workspace object
 *
 * @see getActiveWorkspace()
 */
bool GAUSS::setActiveWorkspace(GEWorkspace *workspace) {
    return this->d->manager_->setCurrent(workspace);
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
__Python__
```py
ge.executeString("x = rndu(3,3)")
ge.executeString("print x")
```
 *
__PHP__
```php
$ge->executeString("x = rndu(3,3)");
$ge->executeString("print x");
```
 *
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
__Python__
```py
ge.executeString("x = rndu(3,3)", myWorkspace)
ge.executeString("print x", myWorkspace)
```
 *
__PHP__
```php
$ge->executeString("x = rndu(3,3)", $myWorkspace);
$ge->executeString("print x", $myWorkspace);
```
 *
 *
 * @param command Expression to execute.
 * @param workspace   Workspace handle
 * @return true on success; false on failure
 *
 * @see compileString(std::string)
 */
bool GAUSS::executeString(std::string command, GEWorkspace *workspace) {
    if (!this->d->manager_->isValidWorkspace(workspace))
        return false;

    ProgramHandle_t *ph = GAUSS_CompileString(workspace->workspace(), removeConst(&command), 0, 0);

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
__Python__
```py
success = ge.executeFile("ols.e")
```
 *
__PHP__
```php
$success = $ge->executeFile("ols.e");
```
 *
 * @param filename        Filename to execute.
 * @return true on success; false on failure
 *
 * @see compileFile(std::string)
 */
bool GAUSS::executeFile(std::string filename) {
    return executeFile(filename, getActiveWorkspace());
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
__Python__
```py
success = ge.executeFile("ols.e", myWorkspace)
```
 *
__PHP__
```php
$success = $ge->executeFile("ols.e", $myWorkspace);
```
 *
 * @param filename        Filename to execute.
 * @param workspace   Workspace handle
 * @return true on success; false on failure
 *
 * @see compileFile(std::string)
 */
bool GAUSS::executeFile(std::string filename, GEWorkspace *workspace) {
    if (endsWithCaseInsensitive(filename, ".gcg"))
        return executeCompiledFile(filename, workspace);
    else if (!this->d->manager_->isValidWorkspace(workspace))
        return false;

    ProgramHandle_t *ph = GAUSS_CompileFile(workspace->workspace(), removeConst(&filename), 0, 0);

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
__Python__
```py
success = ge.executeCompiledFile("example.gcg")
```
 *
__PHP__
```php
$success = $ge->executeCompiledFile("example.gcg");
```
 *
 *
 * @param filename        gcg file to execute.
 * @return true on success; false on failure
 *
 * @see loadWorkspace(std::string)
 * @see loadCompiledFile(std::string)
 */
bool GAUSS::executeCompiledFile(std::string filename) {
    return executeCompiledFile(filename, getActiveWorkspace());
}

/**
 * Executes a compiled gcg file within the GAUSS Engine on a specific workspace. As soon as
 * the file is finished executing it sets the current workspace to what it was before this function
 * was called. If you wish to run this file repeatedly, you can load it first using loadCompiledFile(std::string)
 * and then execute it as many times as you wish with executeProgram(ProgramHandle_t*).
 *
 * Example (Where `myWorkspace` is a GEWorkspace object):
 *
__Python__
```py
success = ge.executeCompiledFile("example.gcg", myWorkspace)
```
 *
__PHP__
```php
$success = $ge->executeCompiledFile("example.gcg", $myWorkspace);
```
 *
 *
 * @param filename        gcg file to execute.
 * @param workspace   Workspace handle
 * @return true on success; false on failure
 *
 * @see loadWorkspace(std::string)
 * @see loadCompiledFile(std::string)
 */
bool GAUSS::executeCompiledFile(std::string filename, GEWorkspace *workspace) {
    if (!this->d->manager_->isValidWorkspace(workspace))
        return false;

    ProgramHandle_t *ph = GAUSS_LoadCompiledFile(workspace->workspace(), removeConst(&filename));

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
__Python__
```py
ph = ge.compileString("print \"Hello World!\"")

for i in range(0, 5):
    ge.executeProgram(ph)
```
 *
__PHP__
```php
$ph = $ge->compileString("print \"Hello World!\"");

for ($i = 0; $i < 5; ++$i)
    $ge->executeProgram($ph);
```
 * results in output:
```
Hello World!
Hello World!
Hello World!
Hello World!
Hello World!
```
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
__Python__
```py
ph = ge.compileString("print \"Hello World!\"", myWorkspace)

for i in range(0, 5):
    ge.executeProgram(ph)
```
 *
__PHP__
```php
$ph = $ge->compileString("print \"Hello World!\"", $myWorkspace);

for ($i = 0; $i < 5; ++$i)
    $ge->executeProgram($ph);
```
 * results in output:
```
Hello World!
Hello World!
Hello World!
Hello World!
Hello World!
```
 *
 * @param command        Code to compile
 * @param workspace    Workspace handle
 * @return        Program handle
 *
 * @see executeProgram(ProgramHandle_t*)
 * @see executeString(std::string)
 * @see freeProgram
 */
ProgramHandle_t* GAUSS::compileString(std::string command, GEWorkspace *workspace) {
    if (!this->d->manager_->isValidWorkspace(workspace))
        return nullptr;

    return GAUSS_CompileString(workspace->workspace(), removeConst(&command), 0, 0);
}

/**
 * Compiles a file and returns a program handle in the active workspace. This can then be followed with a call
 * to executeProgram(ProgramHandle_t*). Note that if you do not care about keeping the program handle,
 * a convenience method executeFile(std::string) is available.
 *
 * Example:
 *
__Python__
```py
ph = ge.compileFile("ols.e")
ge.executeProgram(ph)
```
 *
__PHP__
```php
$ph = $ge->compileFile("ols.e");
$ge->executeProgram(ph);
```
 *
 * @param filename        Filename to compile
 * @return        Program handle
 *
 * @see executeProgram(ProgramHandle_t*)
 * @see executeFile(std::string)
 * @see freeProgram
 */
ProgramHandle_t* GAUSS::compileFile(std::string filename) {
    return compileFile(filename, getActiveWorkspace());
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
__Python__
```py
ph = ge.compileFile("ols.e", myWorkspace)
ge.executeProgram(ph)
```
 *
__PHP__
```php
$ph = $ge->compileFile("ols.e", $myWorkspace);
$ge->executeProgram(ph);
```
 *
 * @param filename        Filename to compile
 * @param workspace    Workspace handle
 * @return        Program handle
 *
 * @see executeProgram(ProgramHandle_t*)
 * @see executeFile(std::string)
 */
ProgramHandle_t* GAUSS::compileFile(std::string filename, GEWorkspace *workspace) {
    if (!this->d->manager_->isValidWorkspace(workspace))
        return nullptr;

    return GAUSS_CompileFile(workspace->workspace(), removeConst(&filename), 0, 0);
}

/**
 * Loads an already compiled file into the active workspace and returns a program handle. This can then
 * be followed with a call to executeProgram(ProgramHandle_t*). Note that if you do not care about
 * keeping the program handle, a convenience method executeCompiledFile(std::string) is available.
 *
 * @param filename        Filename to load
 * @return        Program handle
 *
 * @see executeProgram(ProgramHandle_t*)
 * @see executeCompiledFile(std::string)
 */
ProgramHandle_t* GAUSS::loadCompiledFile(std::string filename) {
    return loadCompiledFile(filename, getActiveWorkspace());
}

/**
 * Loads an already compiled file into a specific workspace and returns a program handle. This can then
 * be followed with a call executeProgram(ProgramHandle_t*). Note that if you do not care about
 * keeping the program handle, a convenience method executeCompiledFile(std::string) is available.
 *
 * @param filename        Filename to load
 * @param workspace    Workspace handle
 * @return        Program handle
 *
 * @see executeProgram(ProgramHandle_t*)
 * @see executeCompiledFile(std::string)
 */
ProgramHandle_t* GAUSS::loadCompiledFile(std::string filename, GEWorkspace *workspace) {
    if (!this->d->manager_->isValidWorkspace(workspace))
        return nullptr;

    return GAUSS_LoadCompiledFile(workspace->workspace(), removeConst(&filename));
}

/**
 * Executes a given program handle that was created with either compileString(std::string), compileFile(std::string),
 * or loadCompiledFile(std::string).
 *
 * Example:
 *
__Python__
```py
ph = ge.compileString("x = rndu(3,3)")
ge.executeProgram(ph)
```
 *
__PHP__
```php
$ph = $ge->compileString("x = rndu(3,3)");
$ge->executeProgram($ph);
```
 *
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

    if (GAUSS_Execute(ph) != 0)
        return false;

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
    WorkspaceHandle_t *workspace = GAUSS_LoadWorkspace(removeConst(&gcgfile));

    if (!workspace)
        return nullptr;

    GEWorkspace *newWh = new GEWorkspace(workspace);

    updateWorkspaceName(newWh);

    // set as active workspace
    setActiveWorkspace(newWh);

    return newWh;
}

/**
 * Returns the current associated name of a workspace according to GAUSS
 *
 * @param workspace Workspace handle
 * @return        Workspace name
 *
 * @see getActiveWorkspace()
 */
std::string GAUSS::getWorkspaceName(GEWorkspace *workspace) const {
    if (!this->d->manager_->isValidWorkspace(workspace))
        return std::string();

    char name[1024];
    memset(&name, 0, sizeof(name));

    GAUSS_GetWorkspaceName(workspace->workspace(), name);

    return std::string(name);
}

/**
 * Request the workspace name from GAUSS, and set the _name_
 * field in the GEWorkspace object.
 *
 * @param workspace        Workspace handle
 *
 * @see getWorkspaceName(GEWorkspace*)
 */
void GAUSS::updateWorkspaceName(GEWorkspace *workspace) {
    std::string wkspName = getWorkspaceName(workspace);

    workspace->setName(wkspName);
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
__Python__
```py
if ge.getSymbolType("x") == GESymType.MATRIX:
    doSomething()
```
 *
__PHP__
```php
if ($ge->getSymbolType("x") == GESymType::MATRIX)
    doSomething();
```
 *
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
__Python__
```py
if ge.getSymbolType("x", myWorkspace) == GESymType.MATRIX:
    doSomething()
```
 *
__PHP__
```php
if ($ge->getSymbolType("x", $myWorkspace) == GESymType::MATRIX)
    doSomething();
```
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
int GAUSS::getSymbolType(std::string name, GEWorkspace *workspace) const {
    if (!this->d->manager_->isValidWorkspace(workspace))
        return -1;

    return GAUSS_GetSymbolType(workspace->workspace(), removeConst(&name));
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
 * You can turn off the error logging to file by inputting an empty std::string for _filename_.
 *
 * @param filename        name of log file.
 * @param mode        **w** to overwrite the contents of the file.\n **a** to append to the contents of the file.
 * @return        True on success, false on failure
 *
 * @see getLogFile()
 */
bool GAUSS::setLogFile(std::string filename, std::string mode) {
    char *logfn_ptr = removeConst(&filename);

    if (filename.empty())
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
    this->d->gauss_home_ = path;
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
__Python__
```py
if not ge.initialize():
    print "Error initializing: " + ge.getLastErrorText()
```
 *
__PHP__
```php
if (!$ge->initialize()) {
    echo "Error initializing: " . $ge->getLastErrorText() . PHP_EOL;
}
```
 *
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
 */
bool GAUSS::isMissingValue(double d) {
    return (GAUSS_IsMissingValue(d) > 0);
}

bool GAUSS::_setSymbol(GESymbol *symbol, std::string name) {
    return _setSymbol(symbol, name, getActiveWorkspace());
}

bool GAUSS::_setSymbol(GESymbol *symbol, std::string name, GEWorkspace *workspace) {
    if (!symbol || name.empty() || !this->d->manager_->isValidWorkspace(workspace))
        return false;

    switch(symbol->type()) {
    case GESymType::SCALAR:
    case GESymType::MATRIX:
        return setSymbol(static_cast<GEMatrix*>(symbol), name, workspace);
        break;
    case GESymType::ARRAY_GAUSS:
        return setSymbol(static_cast<GEArray*>(symbol), name, workspace);
        break;
    case GESymType::STRING:
    case GESymType::STRING_ARRAY:
        return setSymbol(static_cast<GEStringArray*>(symbol), name, workspace);
        break;
    default:
        return false;
    }
}

GESymbol* GAUSS::getSymbol(std::string name) const {
    return getSymbol(name, getActiveWorkspace());
}

GESymbol* GAUSS::getSymbol(std::string name, GEWorkspace *workspace) const {
    if (name.empty() || !this->d->manager_->isValidWorkspace(workspace))
        return 0;

    int type = getSymbolType(name, workspace);

    switch (type) {
    case GESymType::SCALAR:
    case GESymType::MATRIX:
        return getMatrix(name, workspace);
        break;
    case GESymType::ARRAY_GAUSS:
        return getArray(name, workspace);
        break;
    case GESymType::STRING:
    case GESymType::STRING_ARRAY:
        return getStringArray(name, workspace);
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
__Python__
```py
ge.executeString("x = 5")
x = ge.getScalar("x")
print "x = " + str(x)
```
 *
__PHP__
```php
$ge->executeString("x = 5");
$x = $ge->getScalar("x");
echo "\$x = " . $x . PHP_EOL;
```
 * will result in the output:
```
$x = 5
```
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
__Python__
```py
ge.executeString("x = 5", myWorkspace)
x = ge.getScalar("x", myWorkspace)
print "x = " + str(x)
```
 *
__PHP__
```php
$ge->executeString("x = 5", $myWorkspace);
$x = $ge->getScalar("x", $myWorkspace);
echo "\$x = " . $x . PHP_EOL;
```
 * will result in the output:
```
$x = 5
```
 *
 * @param name        Name of symbol
 * @param workspace    Workspace handle
 * @return        Scalar object representing GAUSS symbol
 *
 * @see getScalar(std::string)
 * @see setSymbol(GEMatrix*, std::string)
 * @see setSymbol(GEMatrix*, std::string, GEWorkspace*)
 * @see getMatrixAndClear(std::string)
 * @see getMatrixAndClear(std::string, GEWorkspace*)
 */
double GAUSS::getScalar(std::string name, GEWorkspace *workspace) const {
    if (!this->d->manager_->isValidWorkspace(workspace))
        return 0;

    double d;

    int ret = GAUSS_GetDouble(workspace->workspace(), &d, removeConst(&name));

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
__Python__
```py
ge.executeString("x = 5")
x = ge.getMatrix("x")
print "x = " + str(x.getElement())
```
 *
__PHP__
```php
$ge->executeString("x = 5");
$x = $ge->getMatrix("x");
echo "x = " . $x->getElement() . PHP_EOL;
```
 * will result in the output:
```
x = 5
```
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
__Python__
```py
ge.executeString("x = 5", myWorkspace)
x = ge.getMatrix("x", myWorkspace)
print "x = " + str(x.getElement())
```
 *
__PHP__
```php
$ge->executeString("x = 5", $myWorkspace);
$x = $ge->getMatrix("x", $myWorkspace);
echo "x = " . $x->getElement() . PHP_EOL;
```
 * will result in the output:
```
x = 5
```
 *
 * @param name        Name of GAUSS symbol
 * @param workspace    Workspace handle
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
GEMatrix* GAUSS::getMatrix(std::string name, GEWorkspace *workspace) const {
    if (!this->d->manager_->isValidWorkspace(workspace))
        return nullptr;

    GAUSS_MatrixInfo_t info;
    int ret = GAUSS_GetMatrixInfo(workspace->workspace(), &info, removeConst(&name));

    if (ret)
        return nullptr;

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
__Python__
```py
ge.executeString("x = 5")
x = ge.getMatrixAndClear("x")
print "$x = " + str(x.getElement())
ge.executeString("print \"x = \" x")
```
 *
__PHP__
```php
$ge->executeString("x = 5");
$x = $ge->getMatrixAndClear("x");
echo "\$x = " . $x->getElement() . PHP_EOL;
$ge->executeString("print \"x = \" x");
```
 * will result in the output:
```
$x = 5
x =        0.0000000
```
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
__Python__
```py
ge.executeString("x = 5", myWorkspace)
x = ge.getMatrixAndClear("x", myWorkspace)
print("x = {}".format(str(x.getElement())))
ge.executeString("print \"x = \" x", myWorkspace);
```
 *
__PHP__
```php
$ge->executeString("x = 5", $myWorkspace);
$x = $ge->getMatrixAndClear("x", $myWorkspace);
echo "\$x = " . $x->getElement() . PHP_EOL;
$ge->executeString("print \"x = \" x", $myWorkspace);
```
 * will result in the output:
```
$x = 5
x =        0.0000000
```
 *
 * @param name        Name of GAUSS symbol
 * @param workspace    Workspace handle
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
GEMatrix* GAUSS::getMatrixAndClear(std::string name, GEWorkspace *workspace) const {
    if (!this->d->manager_->isValidWorkspace(workspace))
        return nullptr;

    Matrix_t *gsMat = GAUSS_GetMatrixAndClear(workspace->workspace(), removeConst(&name));

    if (gsMat == nullptr)
        return nullptr;

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
__Python__
```py
ge.executeString("x = 5")
x = ge.getMatrixDirect("x")
print "$x = " + str(x.getitem(0))
ge.executeString("print \"x = \" x");
```
*
__PHP__
```php
$ge->executeString("x = 5");
$x = $ge->getMatrixDirect("x");
echo "\$x = " . $x->getitem(0) . PHP_EOL;
$ge->executeString("print \"x = \" x");
```
* will result in the output:
```
$x = 5
x =        0.0000000
```
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
__Python__
```py
ge.executeString("x = 5", myWorkspace)
x = ge.getMatrixDirect("x", myWorkspace)
print("x = {}".format(str(x.getitem(0))))
ge.executeString("print \"x = \" x", myWorkspace);
```
*
__PHP__
```php
$ge->executeString("x = 5", $myWorkspace);
$x = $ge->getMatrixDirect("x", $myWorkspace);
echo "\$x = " . $x->getitem(0) . PHP_EOL;
$ge->executeString("print \"x = \" x", $myWorkspace);
```
* will result in the output:
```
$x = 5
x =        0.0000000
```
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
doubleArray* GAUSS::getMatrixDirect(std::string name, GEWorkspace* workspace) {
    if (name.empty() || !this->d->manager_->isValidWorkspace(workspace))
        return nullptr;

    GAUSS_MatrixInfo_t info;
    int ret = GAUSS_GetMatrixInfo(workspace->workspace(), &info, removeConst(&name));

    if (ret)
        return nullptr;

    return new doubleArray(info.maddr, info.rows, info.cols);
}

bool GAUSS::_setSymbol(doubleArray *data, std::string name) {
    return _setSymbol(data, name, getActiveWorkspace());
}

bool GAUSS::_setSymbol(doubleArray *data, std::string name, GEWorkspace *workspace) {
    if (!data || name.empty() || !this->d->manager_->isValidWorkspace(workspace))
        return false;

    return moveMatrix(data, 1, data->size(), false, name, workspace);
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
 * @param workspace    Workspace handle
 * @return        Array object
 *
 * @see getArray(std::string)
 * @see setSymbol(GEArray*, std::string)
 * @see setSymbol(GEArray*, std::string, GEWorkspace*)
 * @see getArrayAndClear(std::string)
 * @see getArrayAndClear(std::string, GEWorkspace*)
 */
GEArray* GAUSS::getArray(std::string name, GEWorkspace *workspace) const {
    if (!this->d->manager_->isValidWorkspace(workspace))
        return nullptr;

    Array_t *gsArray = GAUSS_GetArray(workspace->workspace(), removeConst(&name));

    if (gsArray == nullptr)
        return nullptr;

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
 * @param workspace    Workspace handle
 * @return        Array object
 *
 * @see getArrayAndClear(std::string)
 * @see setSymbol(GEArray*, std::string)
 * @see setSymbol(GEArray*, std::string, GEWorkspace*)
 * @see getArray(std::string)
 * @see getArray(std::string, GEWorkspace*)
 */
GEArray* GAUSS::getArrayAndClear(std::string name, GEWorkspace *workspace) const {
    if (!this->d->manager_->isValidWorkspace(workspace))
        return nullptr;

    Array_t *gsArray = GAUSS_GetArrayAndClear(workspace->workspace(), removeConst(&name));

    if (gsArray == nullptr)
        return nullptr;

    return new GEArray(gsArray);
}

/**
 * Retrieve a std::string array from the GAUSS symbol table in the active workspace. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without first
 * calling setSymbol(GEStringArray*, std::string).
 *
 * @param name        Name of GAUSS symbol
 * @return        std::string array object
 *
 * @see getStringArray(std::string, GEWorkspace*)
 * @see setSymbol(GEStringArray*, std::string)
 * @see setSymbol(GEStringArray*, std::string, GEWorkspace*)
 */
GEStringArray* GAUSS::getStringArray(std::string name) const {
    return getStringArray(name, getActiveWorkspace());
}

/**
 * Retrieve a std::string array from the GAUSS symbol table in workspace _wh_. This will be a copy of the symbol
 * from the symbol table, and therefore changes made will not be reflected without first
 * calling setSymbol(GEStringArray*, std::string).
 *
 * @param name        Name of GAUSS symbol
 * @param workspace   Workspace handle
 * @return        std::string array object
 *
 * @see getStringArray(std::string)
 * @see setSymbol(GEStringArray*, std::string)
 * @see setSymbol(GEStringArray*, std::string, GEWorkspace*)
 */
GEStringArray* GAUSS::getStringArray(std::string name, GEWorkspace *workspace) const {
    if (!this->d->manager_->isValidWorkspace(workspace))
        return nullptr;

    StringArray_t *gsStringArray = GAUSS_GetStringArray(workspace->workspace(), removeConst(&name));

    if (gsStringArray == nullptr)
        return nullptr;

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
 * @param workspace   Workspace handle
 * @return        std::string object
 *
 * @see getString(std::string)
 * @see setSymbol(std::string, std::string)
 * @see setSymbol(std::string, std::string, GEWorkspace*)
 */
std::string GAUSS::getString(std::string name, GEWorkspace *workspace) const {
    std::string ret;

    if (!this->d->manager_->isValidWorkspace(workspace))
        return ret;

    String_t *gsString = GAUSS_GetString(workspace->workspace(), removeConst(&name));

    if (gsString == nullptr || gsString->stdata == nullptr)
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
__Python__
```py
x = GEMatrix(5.0)
ge.setSymbol(x, "x")
```
 *
__PHP__
```php
$x = new GEMatrix(5);
$ge->setSymbol($x, "x");
```
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
__Python__
```py
x = GEMatrix(5.0)
ge.setSymbol(x, "x", myWorkspace)
```
 *
__PHP__
```php
$x = new GEMatrix(5);
$ge->setSymbol($x, "x", $myWorkspace);
```
 *
 *
 * @param matrix    Matrix object to store in GAUSS symbol table
 * @param name      Name to give newly added symbol
 * @param workspace    Workspace handle
 * @return          True on success, false on failure
 *
 * @see setSymbol(GEMatrix*, std::string)
 * @see getMatrix(std::string)
 * @see getMatrixAndClear(std::string)
 * @see getScalar(std::string)
 */
bool GAUSS::setSymbol(GEMatrix *matrix, std::string name, GEWorkspace *workspace) {
    if (!matrix || name.empty())
        return false;

    if (!this->d->manager_->isValidWorkspace(workspace))
        return false;

    int ret = 0;

    if (!matrix->isComplex() && (matrix->getRows() == 1) && (matrix->getCols() == 1)) {
        ret = GAUSS_PutDouble(workspace->workspace(), matrix->getElement(), removeConst(&name));
    } else {
        std::unique_ptr<Matrix_t> newMat(matrix->toInternal());
        ret = GAUSS_CopyMatrixToGlobal(workspace->workspace(), newMat.get(), removeConst(&name));
    }

    return (ret == GAUSS_SUCCESS);
}

/**
 * Add an array to the active workspace with the specified symbol name.
 *
 * Example:
 *
__Python__
```py
orders = [2, 2, 2]
data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]
a = GEArray(orders, data)
ge.setSymbol(a, "a")
```
 *
__PHP__
```php
$orders = array(2, 2, 2);
$data = array(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
$a = new GEArray($orders, $data);
$ge->setSymbol($a, "a");
```
 *
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
__Python__
```py
orders = [2, 2, 2]
data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]
a = GEArray(orders, data)
ge.setSymbol(a, "a", myWorkspace)
```
 *
__PHP__
```php
$orders = array(2, 2, 2);
$data = array(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
$a = new GEArray($orders, $data);
$ge->setSymbol($a, "a", $myWorkspace);
```
 *
 *
 * @param array        Array object to store in GAUSS symbol table
 * @param name        Name to give newly added symbol
 * @param workspace    Workspace handle
 * @return True on success, false on failure
 *
 * @see setSymbol(GEArray*, std::string)
 * @see getArray(std::string)
 * @see getArrayAndClear(std::string)
 */
bool GAUSS::setSymbol(GEArray *array, std::string name, GEWorkspace *workspace) {
    if (!array || name.empty())
        return false;

    if (!this->d->manager_->isValidWorkspace(workspace))
        return false;

    std::unique_ptr<Array_t> newArray(array->toInternal());

    if (!newArray.get())
        return false;

    return (GAUSS_CopyArrayToGlobal(workspace->workspace(), newArray.get(), removeConst(&name)) == GAUSS_SUCCESS);
}

/**
 * Add a std::string to the active workspace with the specified symbol name.
 *
 * Example:
 *
__Python__
```py
s = "Hello World"
ge.setSymbol(s, "s")
```
 *
__PHP__
```php
$s = "Hello World";
$ge->setSymbol($s, "s");
```
 *
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
__Python__
```py
s = "Hello World"
ge.setSymbol(s, "s", myWorkspace)
```
 *
__PHP__
```php
$s = "Hello World";
$ge->setSymbol($s, "s", $myWorkspace);
```
 *
 *
 * @param str        std::string to add to GAUSS symbol table
 * @param name        Name to give newly added symbol
 * @param workspace    Workspace handle
 * @return True on success, false on failure
 *
 * @see setSymbol(std::string, std::string)
 * @see getString(std::string)
 */
bool GAUSS::setSymbol(std::string str, std::string name, GEWorkspace *workspace) {
    if (name.empty())
        return false;

    if (!this->d->manager_->isValidWorkspace(workspace))
        return false;

    String_t* newStr = this->d->createPermString(str);

    if (!newStr)
        return false;

    return (GAUSS_MoveStringToGlobal(workspace->workspace(), newStr, removeConst(&name)) == GAUSS_SUCCESS);
}

/**
 * Add a std::string array to the active workspace with the specified symbol name.
 *
 * Example:
 *
__Python__
```py
saData = ["one", "two", "three", "four"]
sa = GEStringArray(saData, 2, 2)
ge.setSymbol(sa, "sa")
```
 *
__PHP__
```php
$saData = array("one", "two", "three", "four");
$sa = new GEStringArray(saData, 2, 2);
$ge->setSymbol($sa, "sa");
```
 *
 * @param sa        std::string array to add to GAUSS symbol table
 * @param name        Name to give newly added symbol
 * @return True on success, false on failure
 *
 * @see getStringArray(std::string)
 */
bool GAUSS::setSymbol(GEStringArray *sa, std::string name) {
    return setSymbol(sa, name, getActiveWorkspace());
}

/**
 * Add a std::string array to a specific workspace with the specified symbol name.
 *
 * Example:
 *
 * Given _myWorkspace_ is a GEWorkspace object
 *
__Python__
```py
saData = ["one", "two", "three", "four"]
sa = GEStringArray(saData, 2, 2)
ge.setSymbol(sa, "sa", myWorkspace)
```
 *
__PHP__
```php
$saData = array("one", "two", "three", "four");
$sa = new GEStringArray(saData, 2, 2);
$ge->setSymbol($sa, "sa", $myWorkspace);
```
 *
 * @param sa        std::string array to add to GAUSS symbol table
 * @param name        Name to give newly added symbol
 * @param workspace    Workspace handle
 * @return True on success, false on failure
 *
 * @see getStringArray(std::string)
 */
bool GAUSS::setSymbol(GEStringArray *sa, std::string name, GEWorkspace *workspace) {
    if (!sa || name.empty())
        return false;

    if (!this->d->manager_->isValidWorkspace(workspace))
        return false;

    StringArray_t *newSa = sa->toInternal();

    if (!newSa)
        return false;

    bool ret = (GAUSS_MoveStringArrayToGlobal(workspace->workspace(), newSa, removeConst(&name)) == GAUSS_SUCCESS);

    return ret;
}

/**
* Add a matrix to the active workspace with the specified symbol name.
* This implementation clears the local data after the move is completed.
*
* Example:
*
__Python__
```py
x = GEMatrix(5.0)
ge.moveSymbol(x, "x")
```
*
__PHP__
```php
$x = new GEMatrix(5);
$ge->moveSymbol($x, "x");
```
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
__Python__
```py
x = GEMatrix(5.0)
ge.moveSymbol(x, "x", myWorkspace)
```
*
__PHP__
```php
$x = new GEMatrix(5);
$ge->moveSymbol($x, "x", $myWorkspace);
```
*
*
* @param matrix    Matrix object to store in GAUSS symbol table
* @param name      Name to give newly added symbol
* @param workspace    Workspace handle
* @return          True on success, false on failure
*
* @see moveSymbol(GEMatrix*, std::string)
* @see getMatrix(std::string)
* @see getMatrixAndClear(std::string)
* @see getScalar(std::string)
*/
bool GAUSS::moveSymbol(GEMatrix *matrix, std::string name, GEWorkspace *workspace) {
    bool ret = setSymbol(matrix, name, workspace);

    if (ret)
        matrix->clear();

    return ret;
}

/**
* Add an array to the active workspace with the specified symbol name.
*
* Example:
*
__Python__
```py
orders = [2, 2, 2]
data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]
a = GEArray(orders, data)
ge.moveSymbol(a, "a")
```
*
__PHP__
```php
$orders = array(2, 2, 2);
$data = array(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
$a = new GEArray($orders, $data);
$ge->moveSymbol($a, "a");
```
*
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
__Python__
```py
orders = [2, 2, 2]
data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]
a = GEArray(orders, data)
ge.moveSymbol(a, "a", myWorkspace)
```
*
__PHP__
```php
$orders = array(2, 2, 2);
$data = array(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
$a = new GEArray($orders, $data);
$ge->moveSymbol($a, "a", $myWorkspace);
```
*
*
* @param array        Array object to store in GAUSS symbol table
* @param name        Name to give newly added symbol
* @param workspace    Workspace handle
* @return True on success, false on failure
*
* @see moveSymbol(GEArray*, std::string)
* @see getArray(std::string)
* @see getArrayAndClear(std::string)
*/
bool GAUSS::moveSymbol(GEArray *array, std::string name, GEWorkspace *workspace) {
    bool ret = setSymbol(array, name, workspace);

    if (ret)
        array->clear();

    return ret;
}

/**
* Add a std::string array to the active workspace with the specified symbol name.
* This implementation clears the local data after the move is completed.
*
* Example:
*
__Python__
```py
saData = ["one", "two", "three", "four"]
sa = GEStringArray(saData, 2, 2)
ge.moveSymbol(sa, "sa")
```
*
__PHP__
```php
$saData = array("one", "two", "three", "four");
$sa = new GEStringArray(saData, 2, 2);
$ge->moveSymbol($sa, "sa");
```
*
* @param sa        std::string array to add to GAUSS symbol table
* @param name        Name to give newly added symbol
* @return True on success, false on failure
*
* @see getStringArray(std::string)
*/
bool GAUSS::moveSymbol(GEStringArray *sa, std::string name) {
    return moveSymbol(sa, name, getActiveWorkspace());
}

/**
* Add a std::string array to a specific workspace with the specified symbol name.
* This implementation clears the local data after the move is completed.
*
* Example:
*
* Given _myWorkspace_ is a GEWorkspace object
*
__Python__
```py
saData = ["one", "two", "three", "four"]
sa = GEStringArray(saData, 2, 2)
ge.moveSymbol(sa, "sa", myWorkspace)
```
*
__PHP__
```php
$saData = array("one", "two", "three", "four");
$sa = new GEStringArray(saData, 2, 2);
$ge->moveSymbol($sa, "sa", $myWorkspace);
```
*
* @param sa        std::string array to add to GAUSS symbol table
* @param name        Name to give newly added symbol
* @param workspace    Workspace handle
* @return True on success, false on failure
*
* @see getStringArray(std::string) const
*/
bool GAUSS::moveSymbol(GEStringArray *sa, std::string name, GEWorkspace *workspace) {
    bool ret = setSymbol(sa, name, workspace);

    if (ret)
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
__Python__
```py
x = doubleArray(1)
x.setitem(0, 5.0);
ge.moveMatrix(x.cast(), 1, 1, False, "x")
```
---
__PHP__
```php
$x = new doubleArray(1);
$x->setitem(0, 5.0);
$ge->moveMatrix($x->cast(), 1, 1, false, "x");
```
*
* @param data      doubleArray wrapper containing data to assign to symbol table
* @param rows      Row count
* @param cols      Column count
* @param is_complex   True if data contains complex data, False otherwise
* @param name      Name to give newly added symbol
* @return          True on success, false on failure
*
* @see moveSymbol(GEMatrix*, std::string, GEWorkspace*)
* @see getMatrix(std::string) const
* @see getMatrixAndClear(std::string) const
* @see getScalar(std::string) const
*/
bool GAUSS::moveMatrix(doubleArray *data, int rows, int cols, bool is_complex, std::string name) {
    return moveMatrix(data, rows, cols, is_complex, name, getActiveWorkspace());
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
__Python__
```py
x = doubleArray(2)
x.setitem(0, 5.0);
x.setitem(1, 10.0);
ge.moveMatrix(x.cast(), 2, 1, False, "x", myWorkspace)
```
----
__PHP__
```php
$x = new doubleArray(2);
$x->setitem(0, 5.0);
$x->setitem(1, 10.0);
$ge->moveMatrix($x->cast(), 2, 1, false, "x", $myWorkspace);
```
*
* @param data      doubleArray wrapper containing data to assign to symbol table
* @param rows      Row count
* @param cols      Column count
* @param is_complex   True if data contains complex data, False otherwise
* @param name      Name to give newly added symbol
* @param workspace        Workspace to assign symbol to
* @return          True on success, false on failure
*
* @see moveMatrix(double*,int,int,bool,std::string)
* @see getMatrix(std::string)
* @see getMatrixAndClear(std::string)
*/
bool GAUSS::moveMatrix(doubleArray *data, int rows, int cols, bool is_complex, std::string name, GEWorkspace *workspace) {
    if (!data || name.empty() || !this->d->manager_->isValidWorkspace(workspace))
        return false;

    int ret = GAUSS_AssignFreeableMatrix(workspace->workspace(), rows, cols, is_complex, data->data(), removeConst(&name));

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
    kOutputStore = std::string();
}

void GAUSS::clearErrorOutput() {
    kErrorStore = std::string();
}

std::string GAUSS::getOutput() {
    if (!GAUSS::outputModeManaged())
        return std::string();

    std::string ret;
    ret.swap(kOutputStore);

    return ret;
}

std::string GAUSS::getErrorOutput() {
    if (!GAUSS::outputModeManaged())
        return std::string();

    std::string ret;
    ret.swap(kErrorStore);

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
        kOutputStore.append(output);
    } else if (GAUSS::outputFunc_) {
        GAUSS::outputFunc_->invoke(std::string(output));
    } else {
        fputs(output, stdout);
    }
}

void GAUSS::internalHookError(char *output) {
    if (GAUSS::outputModeManaged()) {
        kErrorStore.append(output);
    } else if (GAUSS::errorFunc_) {
        GAUSS::errorFunc_->invoke(std::string(output));
    } else {
        fputs(output, stderr);
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
__Python__
```py
class Output(IGEProgramOutput):
    def invoke(self, message):
        print message,

out = Output()
out.thisown = 0
ge.setProgramOutputAll(out)
ge.executeString("rndseed 12345")
ge.executeString("rndu(3, 3)")  # Will produce valid output
ge.executeString("y")           # Will produce error: y does not exist
```
 *
__PHP__
```php
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
```
 *
 *
 * will result in the output:
```
      0.90483859        0.44540096        0.76257185
      0.12938459        0.50966033       0.062034276
      0.70726755       0.077567409        0.83558391

Undefined symbols:
    y
```
 *
 * @param func        User-defined output function.
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
__Python__
```py
class Output(IGEProgramOutput):
    def invoke(self, message):
        print message,

out = Output()
out.thisown = 0

ge.setProgramOutputAll(out)
ge.executeString("rndseed 12345")
ge.executeString("rndu(3, 3)")  # Will produce valid output
ge.executeString("y")           # Will produce error: y does not exist
```
 *
__PHP__
```php
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
```
 *
 *
 * will result in the output:
```
      0.90483859        0.44540096        0.76257185
      0.12938459        0.50966033       0.062034276
      0.70726755       0.077567409        0.83558391
```
 *
 * @param func        User-defined output function.
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
__Python__
```py
class Output(IGEProgramOutput):
    def invoke(self, message):
        print message,

out = Output()
out.thisown = 0

ge.setProgramErrorOutput(out)
ge.executeString("y")           # Will produce error: y does not exist
```
 *
__PHP__
```php
class Output extends IGEProgramOutput {
    function invoke($message) {
        echo $message;
    }
}

$out = new Output();
$out->thisown = 0;

$ge->setProgramErrorOutput($out);
$ge->executeString("y");        // Will produce error: y does not exist
```
 *
 *
 * will result in the output:
```
Undefined symbols:
    y
```
 *
 * @param func        User-defined output function.
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
__Python__
```py
class FlushOutput(IGEProgramFlushOutput):
    def invoke(self):
        print "A flush was requested."

flush = FlushOutput()
flush.thisown = 0

ge.setProgramFlushOutput(flush)
ge.executeString("print /flush \"Hello World!\"")
```
 *
__PHP__
```php
class FlushOutput extends IGEProgramFlushOutput {
    function invoke() {
        echo "A flush was requested.";
    }
}

$flush = new FlushOutput();
$flush->thisown = 0;

$ge->setProgramFlushOutput($flush);
$ge->executeString("print /flush \"Hello World!\"");
```
 *
 *
 * will result in the output:
```
Hello World!
A flush was requested.
```
 *
 * @param func        User-defined flush output function.
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
 * #### GAUSS commands which activate this callback
 * - `cons`
 *
__Python__
```py
class StringInput(IGEProgramInputString):
    # The callback does not return a std::string directly, rather through a method call.
    def invoke(self, length) {
        self.setValue("Hello World!")

strCallback = StringInput();
strCallback.thisown = 0;

ge.setProgramInputString(strCallback)
ge.executeString("s = cons")

s = ge.getString("s")
print "s = " + s
```
 *
__PHP__
```php
class StringInput extends IGEProgramInputString {
    // The callback does not return a std::string directly, rather through a method call.
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
```
 *
 *
 * will result in the output:
```
 * s = Hello World!
```
 *
 * @param func        User-defined output function.
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
 * #### GAUSS commands which activate this callback
 * - `key`
 *
__Python__
```py
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
```
 *
__PHP__
```php
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
```
 *
 *
 * will result in the output:
```
k = 99
```
 *
 * @param func        User-defined input function.
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
 * #### GAUSS commands which activate this callback
 * - `keyw`
 *
__Python__
```py
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
```
 *
__PHP__
```php
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
```
 *
 *
 * will result in the output:
```
k = 99
```
 *
 * @param func        User-defined input function.
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
 * #### GAUSS commands which activate this callback
 * - `keyav`
 *
__Python__
```py
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
```
 *
__PHP__
```php
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
```
 * results in output:
```
Input check requested
Key available
```
 *
 * @param func        User-defined input function.
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

GAUSS::~GAUSS() {
}

bool GAUSSPrivate::managedOutput_ = true;

GAUSSPrivate::GAUSSPrivate(const std::string &homePath) {
    this->gauss_home_ = homePath;
    this->manager_ = new WorkspaceManager;
}

GAUSSPrivate::~GAUSSPrivate() {
    delete this->manager_;
}

String_t* GAUSSPrivate::createPermString(const std::string &data) {
    String_t *newStr = GAUSS_MallocString_t();

    size_t len = data.size() + 1;
    newStr->stdata = (char*)GAUSS_Malloc(len);
    strncpy(newStr->stdata, data.c_str(), len);
    newStr->length = len;
    newStr->freeable = TRUE;

    return newStr;
}
