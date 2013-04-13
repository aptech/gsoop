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
