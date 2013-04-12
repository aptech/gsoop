/* -------------------------------------------------------------------- */
/* Copyright (c) 1997-2003 Aptech Systems, Inc.                         */
/*   All Rights Reserved Worldwide                                      */
/*                                                                      */
/* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF APTECH SYSTEMS, INC.  */
/* The copyright notice above does not evidence any                     */
/* actual or intended publication of such source code.                  */
/* -------------------------------------------------------------------- */


#ifndef _MTENG_H_
#define _MTENG_H_

#ifdef	__cplusplus
extern "C" {
#endif



/*
        A set of useful functions and macros for pthreads programs.
*/

#include <pthread.h>

#define _POSIX_C_SOURCE 199506L

/* Some useful constants */
#ifndef NULL
#define NULL           ((void*)0)
#endif
#ifndef TRUE
#define TRUE           1
#endif
#ifndef FALSE
#define FALSE          0
#endif

#define RRWLock thread_rrwlock_t
#define RRWLock_init(A) thread_rrwlock_init(&(A), NULL)
#define RRWLock_read_lock(A) thread_rrw_rdlock(&(A))
#define RRWLock_write_lock(A) thread_rrw_wrlock(&(A))
#define RRWLock_unlock(A) thread_rrw_unlock(&(A))
#define RRWLock_destroy(A) thread_rrwlock_destroy(&(A))

///#if OS_VER == MSWIN32 && GDT_SIZE_SIZE_T == 4
#if OS_VER == MSWIN32
extern pthread_t gauss_null_tid;
extern void *gauss_thread_id(void);
#endif

/*
   ================================================================
                        General List
   ================================================================
*/

typedef struct _list_t {
        void            *frst;
        struct _list_t  *next;
} list_t;

extern          list_t  *cons(void *, void *);
extern          int     empty(list_t *);
extern          void    *first(list_t *);
extern          list_t  *rest(list_t *);

/*
   ================================================================
                        Recursive Readers / Writer Lock
   ================================================================
*/


typedef struct {
  pthread_mutex_t       lock;           /* lock for structure           */
  pthread_cond_t        readers;        /* waiting readers              */
  pthread_cond_t        writers;        /* waiting writers              */
  int                   nreaders;       /* 0:free,>0:readers  */
  int                   waiters;        /* number of waiting writers    */
  int			nwriters;	/* number of recursive write locks */
  pthread_t		owner;		/* Owner when writer */
} thread_rrwlock_t;

typedef int thread_rrwlock_attr_t;       /* Not implemented */

extern  int     thread_rrwlock_init(thread_rrwlock_t *, thread_rrwlock_attr_t *);
extern  int     thread_rrwlock_destroy(thread_rrwlock_t *);
extern  int     thread_rrw_rdlock(thread_rrwlock_t *);
extern  int     thread_rrw_tryrdlock(thread_rrwlock_t *);
extern  int     thread_rrw_wrlock(thread_rrwlock_t *);
extern  int     thread_rrw_trywrlock(thread_rrwlock_t *);
extern  int     thread_rrw_unlock(thread_rrwlock_t *);


/*
   ================================================================
                        Readers / Writer Lock
   ================================================================
*/

/*
   Definitions for rw locks adapted from Sun's SPILT package.
   RWLocks are  not part of the POSIX threads standard, but
   they are part of UNIX98.
*/

#define THREAD_RWLOCK_INITIALIZER \
 {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, \
    PTHREAD_COND_INITIALIZER, 0, 0}

typedef struct {
  pthread_mutex_t       lock;           /* lock for structure           */
  pthread_cond_t        readers;        /* waiting readers              */
  pthread_cond_t        writers;        /* waiting writers              */
  int                   state;          /* -1:writer,0:free,>0:readers  */
  int                   waiters;        /* number of waiting writers    */
} thread_rwlock_t;

typedef int thread_rwlock_attr_t;       /* Not implemented */

extern  int     thread_rwlock_init(thread_rwlock_t *,
                                        thread_rwlock_attr_t *);
extern  int     thread_rwlock_destroy(thread_rwlock_t *);
extern  int     thread_rw_rdlock(thread_rwlock_t *);
extern  int     thread_rw_tryrdlock(thread_rwlock_t *);
extern  int     thread_rw_wrlock(thread_rwlock_t *);
extern  int     thread_rw_trywrlock(thread_rwlock_t *);
extern  int     thread_rw_unlock(thread_rwlock_t *);


/*
   ================================================================
                        Debug Mutex Lock
   ================================================================
*/


typedef struct _sleep_queue_t {
  pthread_t                     tid;
  struct _sleep_queue_t         *next;
} sleep_queue_t;


typedef struct _dmutex {
  pthread_mutex_t       lock;           /* lock for structure   */
  pthread_cond_t        cv;             /* waiting list control */
  char                  *name;          /* The declared name    */
  sleep_queue_t         *sleepers;      /* List of sleepers     */
  int                   n_locked;       /* Number of times locked */
  int                   n_failed;       /* Times woken w/o locking */
  int                   owned;          /* TRUE | FALSE         */
  pthread_t             owner;          /* NULL_TID or TID      */
} pthread_dmutex_t;

typedef struct _dmutex_queue_t {
  struct _dmutex                *mutex;
  struct _dmutex_queue_t        *next;
} dmutex_queue_t;

extern  void     pthread_dmutex_init(pthread_dmutex_t *, pthread_mutexattr_t *);
extern  void     pthread_dmutex_destroy(pthread_dmutex_t *);
extern  int     pthread_dmutex_lock(pthread_dmutex_t *);
extern  int     pthread_dmutex_trylock(pthread_dmutex_t *);
extern  int     pthread_dmutex_unlock(pthread_dmutex_t *);

#define pthread_dmutex_init(m, a) pthread_dmutex_init1(m, a, #m)

/*
   ================================================================
                        Recursive Mutex Lock
   ================================================================
*/

typedef struct  {
  pthread_mutex_t       lock;           /* lock for structure           */
  pthread_cond_t        cv;             /* waiting list control         */
  int                   count;          /* times locked recursively     */
  int                   owned;          /* TRUE | FALSE         */
  pthread_t             owner;          /* NULL_TID or TID      */
} thread_recursive_mutex_t;

extern  void     thread_recursive_mutex_init(thread_recursive_mutex_t *,
                                                 pthread_mutexattr_t *);
extern  void     thread_recursive_mutex_destroy(thread_recursive_mutex_t *);
extern  int     thread_recursive_mutex_lock(thread_recursive_mutex_t *);
extern  int     thread_recursive_mutex_trylock(thread_recursive_mutex_t *);
extern  int     thread_recursive_mutex_unlock(thread_recursive_mutex_t *);

/*
   ================================================================
                                Barrier
   ================================================================
*/



typedef struct {
  pthread_mutex_t       lock;           /* lock for structure           */
  pthread_cond_t        cv;             /* waiting list control         */
  int                   count;          /* Number of threads to wait for*/
  int                   n_sleepers;     /* Number of threads to waiting */
  int                   releasing;      /* Still waking up sleepers     */
} thread_barrier_t;

extern  void     thread_barrier_init(thread_barrier_t *, int);
extern  void     thread_barrier_destroy(thread_barrier_t *);
extern  int     thread_barrier_wait(thread_barrier_t *);
extern  int     thread_barrier_trywait(thread_barrier_t *);


/*
   ================================================================
                                Single Barrier
   ================================================================
*/



typedef struct  {
  pthread_mutex_t       lock;           /* lock for structure           */
  pthread_cond_t        cv;             /* waiting list control         */
  int                   count;          /* Number of threads to post    */
  int                   n_posters;      /* Number of threads who posted */
  int                   n_waiters;      /* Number of threads waiting    */
  int                   releasing;      /* Still waking up sleepers     */
} thread_single_barrier_t;

extern  void     thread_single_barrier_init(thread_single_barrier_t *, int);
extern  void     thread_single_barrier_destroy(thread_single_barrier_t *);
extern  int     thread_single_barrier_wait(thread_single_barrier_t *);
extern  int     thread_single_barrier_post(thread_single_barrier_t *);

/*
   ================================================================
                        Timed Mutex Lock
   ================================================================
*/

typedef struct _timed_mutex {
  pthread_mutex_t       lock;           /* lock for structure           */
  pthread_cond_t        cv;             /* waiting list control         */
  int                   owned;          /* TRUE | FALSE                 */
  pthread_t             owner;          /* NULL_TID or TID              */
  const struct timespec *abstime;       /* absolute time before timeout */
} thread_timed_mutex_t;

extern  void     thread_timed_mutex_init(thread_timed_mutex_t *,
                                                 pthread_mutexattr_t *);
extern  void     thread_timed_mutex_destroy(thread_timed_mutex_t *);
extern  int     thread_timed_mutex_lock(thread_timed_mutex_t *,
                                            const struct timespec *);
extern  int     thread_timed_mutex_trylock(thread_timed_mutex_t *);
extern  void     thread_timed_mutex_unlock(thread_timed_mutex_t *);

/*
   ================================================================
                        Assorted Useful Functions
   ================================================================
*/


void PTHREAD_CREATE(pthread_t *new_thread_ID,
                   const pthread_attr_t *attr,
                   void * (*start_func)(void *), void *arg);

void PTHREAD_ATTR_INIT(pthread_attr_t *a);

void PTHREAD_CONDATTR_INIT(pthread_condattr_t *a);

void PTHREAD_MUTEXATTR_INIT(pthread_mutexattr_t *a);

void delay(int sleep_ms, int cpu_us);

/*-----------------------------------------------------------------------------
 *
 * Library Name: UTIL
 * Module Name:  iobench
 *
 * Designer:    R. C. Sullivan 
 * Programmer:  R. C. Sullivan
 * Date:        Sep 22, 1995
 *
 * History Of Changes:
 *      Name         Date            Description
 *      ----         ----            -----------
 *      RCS     Jan 17, 1996     Inital release
 *
 * Purpose:
 *   To report resource usage statistics that will be correct for
 * programs using threads on a Solaris system.
 *
 * Notes:
 *
 *-----------------------------------------------------------------------------
 */

extern struct prusage prusagebuf_start, prusagebuf_end;
extern int procfd;
extern double real_time, user_time, system_time, trap_time, wait_time;
extern unsigned long minor_pfs, major_pfs, input_blocks, output_blocks, iochars;

void iobench_start();
void iobench_end();
void iobench_report();


#define GAUSS_BAD_ROW			1
#define GAUSS_BAD_COLUMN		2

#define GAUSS_SUCCESS			0

#define GAUSS_SCALAR				16
#define GAUSS_SPARSE				38
#define GAUSS_MATRIX				 6
#define GAUSS_STRING				13
#define GAUSS_STRUCT				19
#define GAUSS_PSTRUCT  			23
#define GAUSS_STRING_ARRAY		15
#define GAUSS_ARRAY				17
#define GAUSS_PROC   			 8
#define GAUSS_OTHER   			99

#define GAUSS_READONLY			1



typedef struct GAUSS_MatrixInfo_s {
	size_t rows;
	size_t cols;
	int complex;
	double *maddr;
} GAUSS_MatrixInfo_t;

typedef struct Matrix_s {
	double *mdata;
	size_t rows;
	size_t cols;
	int complex;
	int freeable;
} Matrix_t;

typedef struct Array_s {
	double *adata;
	size_t dims;
	size_t nelems;
	int complex;
	int freeable;
} Array_t;

typedef struct String_s {
	char *stdata;
	size_t length;
	int freeable;
} String_t;

typedef struct StringElement_s {
	size_t offset;
	size_t length;
} StringElement_t;

typedef struct StringArray_s {
	StringElement_t *table;
	size_t rows;
	size_t cols;
	size_t size;
	size_t baseoffset;
	int freeable;
} StringArray_t;

typedef struct Argument_s {
	int type;
	size_t rows;
	size_t cols;
	int complex;
	size_t length;
	void *data;
	double scalar;
	size_t size;
	int freeable;
} Argument_t;

typedef struct ArgList_s {
	int num;
	Argument_t *desc;
} ArgList_t;



typedef struct WorkspaceHandle_struct {
	void *lock;
	int activeflag;
	int permsflag;
	void *wd;
} WorkspaceHandle_t;



typedef struct ProgramHandle_struct {
	void *lock;
	int activeflag;
	int permsflag;
	void *pd;
} ProgramHandle_t;



extern  pthread_key_t GAUSS_PROGRAM_OUTPUT_KEY;
extern  pthread_key_t GAUSS_FLUSH_PROGRAM_OUTPUT_KEY;
extern  pthread_key_t GAUSS_PROGRAM_INPUT_STRING_KEY;
extern  pthread_key_t GAUSS_PROGRAM_INPUT_CHAR_KEY;
extern  pthread_key_t GAUSS_PROGRAM_INPUT_CHAR_BLOCKING_KEY;
extern  pthread_key_t GAUSS_PROGRAM_INPUT_CHECK_KEY;
extern  pthread_key_t GAUSS_PROGRAM_ERROR_OUTPUT_KEY;
extern  pthread_key_t GAUSS_CURSOR_GET_KEY;

extern  int GAUSS_ProgramInputString( char *buff, int len );
extern  void *GAUSS_GetProgramErrorOutputHook( void );
extern  void *GAUSS_GetProgramOutputHook( void );
extern  void GAUSS_ProgramErrorOutput( char *error_string );
extern  void GAUSS_ProgramOutput( char *string );
extern  double GAUSS_MissingValue(void);
extern  int GAUSS_IsMissingValue(double d);
extern  void GAUSS_MakePathAbsolute(char *path);
extern  time_t GAUSS_SetInterrupt(pthread_t tid);
extern  time_t GAUSS_CheckInterrupt(pthread_t tid);
extern  int GAUSS_ClearInterrupt(pthread_t tid);
extern  int GAUSS_ClearAllInterrupts(void);
extern  int GAUSS_GetFileWorkspaceName(char *gcgfile, char *buff);
extern  char *GAUSS_GetWorkspaceName(WorkspaceHandle_t *wh, char *buff);
extern  char *GAUSS_SetWorkspaceName(WorkspaceHandle_t *wh, char *name);
extern  void GAUSS_Exit(int num);
extern  ArgList_t *GAUSS_CallProc(ProgramHandle_t *ph, char *procname, ArgList_t *args);
extern  ArgList_t *GAUSS_CallProcFreeArgs(ProgramHandle_t *ph, char *procname, ArgList_t *args);
extern  ArgList_t *GAUSS_CreateArgList(void);
extern  char *GAUSS_ErrorText(char *buff, int errnum);
extern  char *GAUSS_GetHome(char *buff);
extern  char *GAUSS_GetHomeVar(char *buff);
extern  char *GAUSS_GetLogFile(char *buff);
extern  FILE *GAUSS_GetLogStream(void);
extern  int GAUSS_AssignFreeableMatrix(WorkspaceHandle_t *wh, size_t rows, size_t cols, int complex, double *address, char *name);
extern  int GAUSS_AssignFreeableArray(WorkspaceHandle_t *wh, size_t dims, int complex, double *address, char *name);
extern  int GAUSS_CopyArgToArg(ArgList_t *targs, int tnum, ArgList_t *sargs, int snum);
extern  int GAUSS_CopyGlobal(WorkspaceHandle_t *twh, char *tname, WorkspaceHandle_t *swh, char *sname);
extern  int GAUSS_CopyMatrixToArg(ArgList_t *args, Matrix_t *mat, int argnum);
extern  int GAUSS_CopyMatrixToGlobal(WorkspaceHandle_t *wh, Matrix_t *mat, char *name);
extern  int GAUSS_CopyArrayToArg(ArgList_t *args, Array_t *ar, int argnum);
extern  int GAUSS_CopyArrayToGlobal(WorkspaceHandle_t *wh, Array_t *ar, char *name);
extern  int GAUSS_CopyStringArrayToArg(ArgList_t *args, StringArray_t *sa, int argnum);
extern  int GAUSS_CopyStringArrayToGlobal(WorkspaceHandle_t *wh, StringArray_t *sa, char *name);
extern  int GAUSS_CopyStringToArg(ArgList_t *args, String_t *st, int argnum);
extern  int GAUSS_CopyStringToGlobal(WorkspaceHandle_t *wh, String_t *st, char *name);
extern  int GAUSS_DeleteArg(ArgList_t *args, int argnum);
extern  int GAUSS_Execute(ProgramHandle_t *ph);
extern  ArgList_t *GAUSS_ExecuteExpression(ProgramHandle_t *ph);
extern  int GAUSS_ProfileExecute(ProgramHandle_t *ph, FILE *profilefp);
extern  int GAUSS_GetArgType(ArgList_t *args, int num);
extern  int GAUSS_GetDouble(WorkspaceHandle_t *wh, double *d, char *name);
extern  int GAUSS_GetError(void);
extern  int GAUSS_GetMatrixInfo(WorkspaceHandle_t *wh, GAUSS_MatrixInfo_t *matinfo, char *name);
extern  int GAUSS_GetSymbolType(WorkspaceHandle_t *wh, char *name);
extern  int GAUSS_GetSysConfig(char *value, char *variable);
extern  int GAUSS_Initialize(void);
extern  int GAUSS_InputChar(void);
extern  int GAUSS_InputCharBlocking(void);
extern  int GAUSS_InputString(char *buff, int len);
extern  int GAUSS_InsertArg(ArgList_t *args, int argnum);
extern  int GAUSS_MoveArgToArg(ArgList_t *targs, int tnum, ArgList_t *sargs, int snum);
extern  int GAUSS_MoveMatrixToArg(ArgList_t *args, Matrix_t *mat, int argnum);
extern  int GAUSS_MoveMatrixToGlobal(WorkspaceHandle_t *wh, Matrix_t *mat, char *name);
extern  int GAUSS_MoveArrayToArg(ArgList_t *args, Array_t *ar, int argnum);
extern  int GAUSS_MoveArrayToGlobal(WorkspaceHandle_t *wh, Array_t *ar, char *name);
extern  int GAUSS_MoveStringArrayToArg(ArgList_t *args, StringArray_t *sa, int argnum);
extern  int GAUSS_MoveStringArrayToGlobal(WorkspaceHandle_t *wh, StringArray_t *sa, char *name);
extern  int GAUSS_MoveStringToArg(ArgList_t *args, String_t *st, int argnum);
extern  int GAUSS_MoveStringToGlobal(WorkspaceHandle_t *wh, String_t *st, char *name);
extern  int GAUSS_PutDouble(WorkspaceHandle_t *wh, double d, char *name);
extern  int GAUSS_PutDoubleInArg(ArgList_t *args, double d, int argnum);
extern  int GAUSS_SaveProgram(ProgramHandle_t *ph, char *fn);
extern  int GAUSS_SaveWorkspace(WorkspaceHandle_t *wh, char *fn);
extern  int GAUSS_SetError(int num);
extern  int GAUSS_SetHomeVar(char *name);
extern  int GAUSS_SetLogFile(char *logfn, char *mode);
extern  int GAUSS_TranslateDataloopFile(char *transfile, char *srcfile);
extern  Matrix_t *GAUSS_ComplexMatrix(size_t rows, size_t cols, double *real, double *imag);
extern  Matrix_t *GAUSS_ComplexMatrixAlias(size_t rows, size_t cols, double *addr);
extern  Array_t *GAUSS_ComplexArray(size_t dims, double *orders, double *real, double *imag);
extern  Array_t *GAUSS_ComplexArrayAlias(size_t dims, double *addr);
extern  Matrix_t *GAUSS_CopyArgToMatrix(ArgList_t *args, int num);
extern  Array_t *GAUSS_CopyArgToArray(ArgList_t *args, int num);
extern  Matrix_t *GAUSS_GetMatrix(WorkspaceHandle_t *wh, char *name);
extern  Array_t *GAUSS_GetArray(WorkspaceHandle_t *wh, char *name);
extern  Matrix_t *GAUSS_GetMatrixAndClear(WorkspaceHandle_t *wh, char *name);
extern  Array_t *GAUSS_GetArrayAndClear(WorkspaceHandle_t *wh, char *name);
extern  Matrix_t *GAUSS_Matrix(size_t rows, size_t cols, double *addr);
extern  Array_t *GAUSS_Array(size_t dims, double *orders, double *addr);
extern  Matrix_t *GAUSS_MatrixAlias(size_t rows, size_t cols, double *addr);
extern  Array_t *GAUSS_ArrayAlias(size_t dims, double *addr);
extern  Matrix_t *GAUSS_MoveArgToMatrix(ArgList_t *args, int num);
extern  Array_t *GAUSS_MoveArgToArray(ArgList_t *args, int num);
extern  ProgramHandle_t *GAUSS_CompileExpression(WorkspaceHandle_t *wh, char *str, int readonlyC, int readonlyE);
extern  ProgramHandle_t *GAUSS_CompileFile(WorkspaceHandle_t *wh, char *fn, int readonlyC, int readonlyE);
extern  ProgramHandle_t *GAUSS_CompileString(WorkspaceHandle_t *wh, char *str, int readonlyC, int readonlyE);
extern  ProgramHandle_t *GAUSS_CompileStringAsFile(WorkspaceHandle_t *wh, char *str, int readonlyC, int readonlyE);
extern  ProgramHandle_t *GAUSS_CreateProgram(WorkspaceHandle_t *wh, int readonlyE);
extern  ProgramHandle_t *GAUSS_LoadCompiledBuffer(WorkspaceHandle_t *wh, char *buff);
extern  ProgramHandle_t *GAUSS_LoadCompiledFile(WorkspaceHandle_t *wh, char *gcgfile);
extern  WorkspaceHandle_t *GAUSS_LoadWorkspace(char *gcgfile);
extern  String_t *GAUSS_CopyArgToString(ArgList_t *args, int num);
extern  String_t *GAUSS_GetString(WorkspaceHandle_t *wh, char *name);
extern  String_t *GAUSS_MoveArgToString(ArgList_t *args, int num);
extern  String_t *GAUSS_String(char *str);
extern  String_t *GAUSS_StringAlias(char *str);
extern  String_t *GAUSS_StringL(char *str, size_t len);
extern  String_t *GAUSS_StringAliasL(char *str, size_t len);
extern  StringArray_t *GAUSS_CopyArgToStringArray(ArgList_t *args, int num);
extern  StringArray_t *GAUSS_GetStringArray(WorkspaceHandle_t *wh, char *name);
extern  StringArray_t *GAUSS_MoveArgToStringArray(ArgList_t *args, int num);
extern  StringArray_t *GAUSS_StringArray(size_t rows, size_t cols, char **strs);
extern  StringArray_t *GAUSS_StringArrayL(size_t rows, size_t cols, char **strs, size_t *lens);
extern  void GAUSS_FreeArgList(ArgList_t *args);
extern  void GAUSS_FreeMatrix(Matrix_t *mat);
extern  void GAUSS_FreeArray(Array_t *ar);
extern  void GAUSS_FreeProgram(ProgramHandle_t *ph);
extern  void GAUSS_FreeString(String_t *str);
extern  void GAUSS_FreeStringArray(StringArray_t *sa);
extern  void GAUSS_FreeWorkspace(WorkspaceHandle_t *wh);
extern  void GAUSS_HookProgramErrorOutput( void (*display_error_string_function)(char *) );
extern  void GAUSS_HookProgramOutput( void (*display_string_function)(char *str) );
extern  void GAUSS_HookFlushProgramOutput( void (*flush_display_function)(void) );
extern  void GAUSS_HookProgramInputChar( int (*get_char_function)(void) );
extern  void GAUSS_HookProgramInputCharBlocking( int (*get_char_blocking_function)(void) );
extern  void GAUSS_HookGetCursorPosition( int (*get_cursor_position_function)(void) );
extern  void GAUSS_HookProgramInputString( int (*get_string_function)(char *, int) );
extern  void GAUSS_HookProgramInputCheck( int (*get_string_function)(void) );
extern  int GAUSS_SetHome(char *path);									 
extern  void GAUSS_SetLogStream(FILE *logfp);
extern  void GAUSS_Shutdown(void);											
extern  WorkspaceHandle_t *GAUSS_CreateWorkspace(char *name);
extern  void GAUSS_SetDotExecute(ProgramHandle_t *ph);
extern  void GAUSS_ClearDotExecute(ProgramHandle_t *ph);
extern  void *GAUSS_Malloc( size_t bytes );
extern  void GAUSS_Free( void *p );
extern  Matrix_t *GAUSS_MallocMatrix_t( void );
extern  Array_t *GAUSS_MallocArray_t( void );
extern  String_t *GAUSS_MallocString_t( void );
extern  StringArray_t *GAUSS_MallocStringArray_t( void );


#ifdef	__cplusplus
}
#endif

#endif	/* _MTENG_H_ */

