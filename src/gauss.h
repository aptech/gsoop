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
    bool setHome(std::string);
    bool setHomeVar(std::string);
    std::string getHome() const;
    std::string getHomeVar() const;
    std::string getLogFile() const;
    bool setLogFile(std::string, std::string);

    // errors
    std::string getLastErrorText() const;
    std::string getErrorText(int) const;
    int getError() const;
    void setError(int);

    // workspaces
    GEWorkspace* createWorkspace(std::string);
    bool destroyWorkspace(GEWorkspace*);
    void destroyAllWorkspaces();
    GEWorkspace* getWorkspace(std::string) const;
    GEWorkspace* getActiveWorkspace() const;
    bool setActiveWorkspace(GEWorkspace*);
    GEWorkspace* loadWorkspace(std::string);
    std::string getWorkspaceName(GEWorkspace*) const;
    void updateWorkspaceName(GEWorkspace*);

    bool saveWorkspace(GEWorkspace*, std::string);
    bool saveProgram(ProgramHandle_t*, std::string);
    std::string translateDataloopFile(std::string srcfile);

    // program execution
    bool executeString(std::string);
    bool executeString(std::string, GEWorkspace*);
    bool executeFile(std::string);
    bool executeFile(std::string, GEWorkspace*);
    bool executeCompiledFile(std::string);
    bool executeCompiledFile(std::string, GEWorkspace*);
    ProgramHandle_t* compileString(std::string);
    ProgramHandle_t* compileString(std::string, GEWorkspace*);
    ProgramHandle_t* compileFile(std::string);
    ProgramHandle_t* compileFile(std::string, GEWorkspace*);
    ProgramHandle_t* loadCompiledFile(std::string);
    ProgramHandle_t* loadCompiledFile(std::string, GEWorkspace*);
    bool executeProgram(ProgramHandle_t*);
    void freeProgram(ProgramHandle_t*);

    std::string makePathAbsolute(std::string);
    std::string programInputString();
    int getSymbolType(std::string) const;
    int getSymbolType(std::string, GEWorkspace*) const;

    double getScalar(std::string) const;
    double getScalar(std::string, GEWorkspace*) const;
    GEMatrix* getMatrix(std::string) const;
    GEMatrix* getMatrix(std::string, GEWorkspace*) const;
    GEMatrix* getMatrixAndClear(std::string) const;
    GEMatrix* getMatrixAndClear(std::string, GEWorkspace*) const;
    GEArray* getArray(std::string) const;
    GEArray* getArray(std::string, GEWorkspace*) const;
    GEArray* getArrayAndClear(std::string) const;
    GEArray* getArrayAndClear(std::string, GEWorkspace*) const;
    std::string getString(std::string) const;
    std::string getString(std::string, GEWorkspace*) const;
    GEStringArray* getStringArray(std::string) const;
    GEStringArray* getStringArray(std::string, GEWorkspace*) const;

    bool setSymbol(GEMatrix*, std::string);
    bool setSymbol(GEMatrix*, std::string, GEWorkspace*);
    bool setSymbol(GEArray*, std::string);
    bool setSymbol(GEArray*, std::string, GEWorkspace*);
    bool setSymbol(std::string, std::string);
    bool setSymbol(std::string, std::string, GEWorkspace*);
    bool setSymbol(GEStringArray*, std::string);
    bool setSymbol(GEStringArray*, std::string, GEWorkspace*);

	bool moveSymbol(GEMatrix*, std::string);
	bool moveSymbol(GEMatrix*, std::string, GEWorkspace*);
	bool moveSymbol(GEArray*, std::string);
	bool moveSymbol(GEArray*, std::string, GEWorkspace*);
	bool moveSymbol(GEStringArray*, std::string);
	bool moveSymbol(GEStringArray*, std::string, GEWorkspace*);

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
