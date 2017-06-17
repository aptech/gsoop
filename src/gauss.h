#ifndef GAUSS_H
#define GAUSS_H

#ifdef SWIG
#define GAUSS_EXPORT 
#endif

#ifndef GAUSS_EXPORT
#ifdef _WIN32
#ifdef GAUSS_LIBRARY
#    define GAUSS_EXPORT _declspec(dllexport)
#else
#    define GAUSS_EXPORT _declspec(dllimport)
#endif
#else
#    define GAUSS_EXPORT __attribute__((visibility("default")))
#endif
#endif

#include <stdio.h>
#include <cstdlib>
#include <utility>
#include <mteng.h>
#include <string>

class doubleArray;
class GESymbol;
class GEArray;
class GEMatrix;
class GEStringArray;
class GEWorkspace;
class WorkspaceManager;
class IGEProgramOutput;
class IGEProgramFlushOutput;
class IGEProgramInputString;
class IGEProgramInputChar;
class IGEProgramInputCheck;
class GAUSSPrivate;

#define UNUSED_VAR(x) (void*)x;

/**
 * Main class that must be instantiated to interface with the GAUSS Engine.
 * While not enforced, only __one__ instance of this class should be present. If threading is an issue,
 * consider the use of multiple workspaces, as each can be accessed in parallel safely.
 *
 * #### Python ####
 * ~~~{.py}
ge = GAUSS() # Use MTENGHOME environment variable value as GAUSS home location
ge = GAUSS("MYHOMEVAR") # Use specified environment variable for GAUSS home location
ge = GAUSS("/home/user/mteng", False) # Use specified paths for GAUSS home location
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$ge = new GAUSS(); // Use MTENGHOME environment variable value as GAUSS home location
$ge = new GAUSS("MYHOMEVAR"); // Use specified environment variable for GAUSS home location
$ge = new GAUSS("/home/user/mteng", false); // Use specified paths for GAUSS home location
 * ~~~
 *
 */
class GAUSS_EXPORT GAUSS {
public:
    GAUSS();
    GAUSS(std::string, bool isEnvVar = true);
    virtual ~GAUSS();

    // init
    bool initialize();
    void shutdown();

    // home
    bool setHome(std::string path);
    bool setHomeVar(std::string environment);
    std::string getHome() const;
    std::string getHomeVar() const;
    std::string getLogFile() const;
    bool setLogFile(std::string filename, std::string mode);

    // errors
    std::string getLastErrorText() const;
    std::string getErrorText(int) const;
    int getError() const;
    void setError(int);

    // workspaces
    GEWorkspace* createWorkspace(std::string name);
    bool destroyWorkspace(GEWorkspace *workspace);
    void destroyAllWorkspaces();
    GEWorkspace* getWorkspace(std::string name) const;
    GEWorkspace* getActiveWorkspace() const;
    bool setActiveWorkspace(GEWorkspace *workspace);
    GEWorkspace* loadWorkspace(std::string filename);
    std::string getWorkspaceName(GEWorkspace *workspace) const;
    void updateWorkspaceName(GEWorkspace *workspace);

    bool saveWorkspace(GEWorkspace *workspace, std::string filename);
    bool saveProgram(ProgramHandle_t *programHandle, std::string filename);
    std::string translateDataloopFile(std::string filename);

    // program execution
    bool executeString(std::string code);
    bool executeString(std::string code, GEWorkspace *workspace);
    bool executeFile(std::string filename);
    bool executeFile(std::string filename, GEWorkspace *workspace);
    bool executeCompiledFile(std::string filename);
    bool executeCompiledFile(std::string filename, GEWorkspace *workspace);
    ProgramHandle_t* compileString(std::string code);
    ProgramHandle_t* compileString(std::string code, GEWorkspace *workspace);
    ProgramHandle_t* compileFile(std::string filename);
    ProgramHandle_t* compileFile(std::string filename, GEWorkspace *workspace);
    ProgramHandle_t* loadCompiledFile(std::string filename);
    ProgramHandle_t* loadCompiledFile(std::string filename, GEWorkspace *workspace);
    bool executeProgram(ProgramHandle_t *programHandle);
    void freeProgram(ProgramHandle_t *programHandle);

    std::string makePathAbsolute(std::string path);
    std::string programInputString();
    int getSymbolType(std::string name) const;
    int getSymbolType(std::string name, GEWorkspace *workspace) const;

    double getScalar(std::string name) const;
    double getScalar(std::string name, GEWorkspace *workspace) const;
    GEMatrix* getMatrix(std::string name) const;
    GEMatrix* getMatrix(std::string name, GEWorkspace *workspace) const;
    GEMatrix* getMatrixAndClear(std::string name) const;
    GEMatrix* getMatrixAndClear(std::string name, GEWorkspace *workspace) const;
    GEArray* getArray(std::string name) const;
    GEArray* getArray(std::string name, GEWorkspace *workspace) const;
    GEArray* getArrayAndClear(std::string name) const;
    GEArray* getArrayAndClear(std::string name, GEWorkspace *workspace) const;
    std::string getString(std::string name) const;
    std::string getString(std::string name, GEWorkspace *workspace) const;
    GEStringArray* getStringArray(std::string name) const;
    GEStringArray* getStringArray(std::string name, GEWorkspace *workspace) const;

    bool setSymbol(GEMatrix*, std::string name);
    bool setSymbol(GEMatrix*, std::string name, GEWorkspace *workspace);
    bool setSymbol(GEArray*, std::string name);
    bool setSymbol(GEArray*, std::string name, GEWorkspace *workspace);
    bool setSymbol(std::string, std::string name);
    bool setSymbol(std::string, std::string name, GEWorkspace *workspace);
    bool setSymbol(GEStringArray*, std::string name);
    bool setSymbol(GEStringArray*, std::string name, GEWorkspace *workspace);

	bool moveSymbol(GEMatrix*, std::string name);
	bool moveSymbol(GEMatrix*, std::string name, GEWorkspace *workspace);
	bool moveSymbol(GEArray*, std::string name);
	bool moveSymbol(GEArray*, std::string name, GEWorkspace *workspace);
	bool moveSymbol(GEStringArray *symbol, std::string name);
	bool moveSymbol(GEStringArray *symbol, std::string name, GEWorkspace *workspace);

    bool moveMatrix(doubleArray *data, int rows, int cols, bool complex, std::string name);
    bool moveMatrix(doubleArray *data, int rows, int cols, bool complex, std::string name, GEWorkspace* workspace);

    doubleArray* getMatrixDirect(std::string name);
    doubleArray* getMatrixDirect(std::string name, GEWorkspace* workspace);

    bool _setSymbol(doubleArray *data, std::string name);
    bool _setSymbol(doubleArray *data, std::string name, GEWorkspace *workspace);

    bool _setSymbol(GESymbol *symbol, std::string name);
    bool _setSymbol(GESymbol *symbol, std::string name, GEWorkspace *workspace);

    GESymbol* getSymbol(std::string name) const;
    GESymbol* getSymbol(std::string name, GEWorkspace *workspace) const;

    static bool isMissingValue(double);

    static void internalHookOutput(char *output);
    static void internalHookError(char *output);
    static void internalHookFlush();
    static int internalHookInputString(char *buf, int len);
    static int internalHookInputChar();
    static int internalHookInputBlockingChar();
    static int internalHookInputCheck();

    std::string getOutput();
    void clearOutput();
    std::string getErrorOutput();
    void clearErrorOutput();

    static void setOutputModeManaged(bool managed);
    static bool outputModeManaged();

    static void setProgramOutputAll(IGEProgramOutput *func);
    static void setProgramOutput(IGEProgramOutput *func);
    static void setProgramErrorOutput(IGEProgramOutput *func);
    static void setProgramFlushOutput(IGEProgramFlushOutput *func);
    static void setProgramInputString(IGEProgramInputString *func);
    static void setProgramInputChar(IGEProgramInputChar *func);
    static void setProgramInputCharBlocking(IGEProgramInputChar *func);
    static void setProgramInputCheck(IGEProgramInputCheck *func);

protected:
    // Keep these private, since we have accessors in place for these.
    void resetHooks();
    void setHookOutput(void (*display_string_function)(char *str));
    void setHookProgramErrorOutput(void (*display_error_string_function)(char *));
    void setHookProgramOutput(void (*display_string_function)(char *str));
    void setHookFlushProgramOutput(void (*flush_display_function)(void));
    void setHookProgramInputChar(int (*get_char_function)(void));
    void setHookProgramInputBlockingChar(int (*get_char_blocking_function)(void));
    void setHookGetCursorPosition(int (*get_cursor_position_function)(void));
    void setHookProgramInputString(int (*get_string_function)(char *, int));
    void setHookProgramInputCheck(int (*get_string_function)(void));

private:
    void Init(std::string homePath);

    GAUSSPrivate *d;

    static IGEProgramOutput *outputFunc_;
    static IGEProgramOutput *errorFunc_;
    static IGEProgramFlushOutput *flushFunc_;
    static IGEProgramInputString *inputStringFunc_;
    static IGEProgramInputChar *inputCharFunc_;
    static IGEProgramInputChar *inputBlockingCharFunc_;
    static IGEProgramInputCheck *inputCheckFunc_;
};


#endif // GAUSS_H
