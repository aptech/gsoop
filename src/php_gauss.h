/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.10
 * 
 * This file is not intended to be easily readable and contains a number of 
 * coding conventions designed to improve portability and efficiency. Do not make
 * changes to this file unless you know what you are doing--modify the SWIG 
 * interface file instead. 
 * ----------------------------------------------------------------------------- */

#ifndef PHP_GAUSS_H
#define PHP_GAUSS_H

extern zend_module_entry gauss_module_entry;
#define phpext_gauss_ptr &gauss_module_entry

#ifdef PHP_WIN32
# define PHP_GAUSS_API __declspec(dllexport)
#else
# define PHP_GAUSS_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(gauss);
PHP_MSHUTDOWN_FUNCTION(gauss);
PHP_RINIT_FUNCTION(gauss);
PHP_RSHUTDOWN_FUNCTION(gauss);
PHP_MINFO_FUNCTION(gauss);

ZEND_NAMED_FUNCTION(_wrap_new_DoubleVector);
ZEND_NAMED_FUNCTION(_wrap_DoubleVector_size);
ZEND_NAMED_FUNCTION(_wrap_DoubleVector_capacity);
ZEND_NAMED_FUNCTION(_wrap_DoubleVector_reserve);
ZEND_NAMED_FUNCTION(_wrap_DoubleVector_clear);
ZEND_NAMED_FUNCTION(_wrap_DoubleVector_push);
ZEND_NAMED_FUNCTION(_wrap_DoubleVector_is_empty);
ZEND_NAMED_FUNCTION(_wrap_DoubleVector_pop);
ZEND_NAMED_FUNCTION(_wrap_DoubleVector_get);
ZEND_NAMED_FUNCTION(_wrap_DoubleVector_set);
ZEND_NAMED_FUNCTION(_wrap_new_DoubleDoubleVector);
ZEND_NAMED_FUNCTION(_wrap_DoubleDoubleVector_size);
ZEND_NAMED_FUNCTION(_wrap_DoubleDoubleVector_capacity);
ZEND_NAMED_FUNCTION(_wrap_DoubleDoubleVector_reserve);
ZEND_NAMED_FUNCTION(_wrap_DoubleDoubleVector_clear);
ZEND_NAMED_FUNCTION(_wrap_DoubleDoubleVector_push);
ZEND_NAMED_FUNCTION(_wrap_DoubleDoubleVector_is_empty);
ZEND_NAMED_FUNCTION(_wrap_DoubleDoubleVector_pop);
ZEND_NAMED_FUNCTION(_wrap_DoubleDoubleVector_get);
ZEND_NAMED_FUNCTION(_wrap_DoubleDoubleVector_set);
ZEND_NAMED_FUNCTION(_wrap_new_FloatVector);
ZEND_NAMED_FUNCTION(_wrap_FloatVector_size);
ZEND_NAMED_FUNCTION(_wrap_FloatVector_capacity);
ZEND_NAMED_FUNCTION(_wrap_FloatVector_reserve);
ZEND_NAMED_FUNCTION(_wrap_FloatVector_clear);
ZEND_NAMED_FUNCTION(_wrap_FloatVector_push);
ZEND_NAMED_FUNCTION(_wrap_FloatVector_is_empty);
ZEND_NAMED_FUNCTION(_wrap_FloatVector_pop);
ZEND_NAMED_FUNCTION(_wrap_FloatVector_get);
ZEND_NAMED_FUNCTION(_wrap_FloatVector_set);
ZEND_NAMED_FUNCTION(_wrap_new_IntVector);
ZEND_NAMED_FUNCTION(_wrap_IntVector_size);
ZEND_NAMED_FUNCTION(_wrap_IntVector_capacity);
ZEND_NAMED_FUNCTION(_wrap_IntVector_reserve);
ZEND_NAMED_FUNCTION(_wrap_IntVector_clear);
ZEND_NAMED_FUNCTION(_wrap_IntVector_push);
ZEND_NAMED_FUNCTION(_wrap_IntVector_is_empty);
ZEND_NAMED_FUNCTION(_wrap_IntVector_pop);
ZEND_NAMED_FUNCTION(_wrap_IntVector_get);
ZEND_NAMED_FUNCTION(_wrap_IntVector_set);
ZEND_NAMED_FUNCTION(_wrap_new_StringVector);
ZEND_NAMED_FUNCTION(_wrap_StringVector_size);
ZEND_NAMED_FUNCTION(_wrap_StringVector_capacity);
ZEND_NAMED_FUNCTION(_wrap_StringVector_reserve);
ZEND_NAMED_FUNCTION(_wrap_StringVector_clear);
ZEND_NAMED_FUNCTION(_wrap_StringVector_push);
ZEND_NAMED_FUNCTION(_wrap_StringVector_is_empty);
ZEND_NAMED_FUNCTION(_wrap_StringVector_pop);
ZEND_NAMED_FUNCTION(_wrap_StringVector_get);
ZEND_NAMED_FUNCTION(_wrap_StringVector_set);
ZEND_NAMED_FUNCTION(_wrap_new_StringStringVector);
ZEND_NAMED_FUNCTION(_wrap_StringStringVector_size);
ZEND_NAMED_FUNCTION(_wrap_StringStringVector_capacity);
ZEND_NAMED_FUNCTION(_wrap_StringStringVector_reserve);
ZEND_NAMED_FUNCTION(_wrap_StringStringVector_clear);
ZEND_NAMED_FUNCTION(_wrap_StringStringVector_push);
ZEND_NAMED_FUNCTION(_wrap_StringStringVector_is_empty);
ZEND_NAMED_FUNCTION(_wrap_StringStringVector_pop);
ZEND_NAMED_FUNCTION(_wrap_StringStringVector_get);
ZEND_NAMED_FUNCTION(_wrap_StringStringVector_set);
ZEND_NAMED_FUNCTION(_wrap_new_GAUSS);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_initialize);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_shutdown);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_setHome);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_setHomeVar);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_getHome);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_getHomeVar);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_getLogFile);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_setLogFile);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_getLastErrorText);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_getErrorText);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_getError);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_setError);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_createWorkspace);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_destroyWorkspace);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_destroyAllWorkspaces);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_getWorkspace);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_getActiveWorkspace);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_setActiveWorkspace);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_loadWorkspace);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_getWorkspaceName);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_updateWorkspaceName);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_saveWorkspace);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_saveProgram);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_translateDataloopFile);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_executeString);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_executeFile);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_executeCompiledFile);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_compileString);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_compileFile);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_loadCompiledFile);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_executeProgram);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_freeProgram);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_makePathAbsolute);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_programInputString);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_getSymbolType);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_getScalar);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_getMatrix);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_getMatrixAndClear);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_getArray);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_getArrayAndClear);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_getString);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_getStringArray);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_setSymbol);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_isMissingValue);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_kMissingValue_set);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_kMissingValue_get);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_internalHookOutput);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_internalHookError);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_internalHookFlush);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_internalHookInputString);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_internalHookInputChar);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_internalHookInputBlockingChar);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_internalHookInputCheck);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_setProgramOutputAll);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_setProgramOutput);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_setProgramErrorOutput);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_setProgramFlushOutput);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_setProgramInputString);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_setProgramInputChar);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_setProgramInputCharBlocking);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_setProgramInputCheck);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_outputFunc__set);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_outputFunc__get);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_errorFunc__set);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_errorFunc__get);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_flushFunc__set);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_flushFunc__get);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_inputStringFunc__set);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_inputStringFunc__get);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_inputCharFunc__set);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_inputCharFunc__get);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_inputBlockingCharFunc__set);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_inputBlockingCharFunc__get);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_inputCheckFunc__set);
ZEND_NAMED_FUNCTION(_wrap_GAUSS_inputCheckFunc__get);
ZEND_NAMED_FUNCTION(_wrap_GESymbol_getRows);
ZEND_NAMED_FUNCTION(_wrap_GESymbol_getCols);
ZEND_NAMED_FUNCTION(_wrap_GESymbol_isComplex);
ZEND_NAMED_FUNCTION(_wrap_GESymbol_size);
ZEND_NAMED_FUNCTION(_wrap_GESymbol_clear);
ZEND_NAMED_FUNCTION(_wrap_GESymbol_toString);
ZEND_NAMED_FUNCTION(_wrap_GESymbol___toString);
ZEND_NAMED_FUNCTION(_wrap_new_GEArray);
ZEND_NAMED_FUNCTION(_wrap_GEArray_getPlane);
ZEND_NAMED_FUNCTION(_wrap_GEArray_getVector);
ZEND_NAMED_FUNCTION(_wrap_GEArray_getElement);
ZEND_NAMED_FUNCTION(_wrap_GEArray_setElement);
ZEND_NAMED_FUNCTION(_wrap_GEArray_getData);
ZEND_NAMED_FUNCTION(_wrap_GEArray_getImagData);
ZEND_NAMED_FUNCTION(_wrap_GEArray_getOrders);
ZEND_NAMED_FUNCTION(_wrap_GEArray_getDimensions);
ZEND_NAMED_FUNCTION(_wrap_GEArray_size);
ZEND_NAMED_FUNCTION(_wrap_GEArray_toString);
ZEND_NAMED_FUNCTION(_wrap_GEArray_clear);
ZEND_NAMED_FUNCTION(_wrap_new_GEMatrix);
ZEND_NAMED_FUNCTION(_wrap_GEMatrix_setElement);
ZEND_NAMED_FUNCTION(_wrap_GEMatrix_getElement);
ZEND_NAMED_FUNCTION(_wrap_GEMatrix_getData);
ZEND_NAMED_FUNCTION(_wrap_GEMatrix_getImagData);
ZEND_NAMED_FUNCTION(_wrap_GEMatrix_clear);
ZEND_NAMED_FUNCTION(_wrap_GEMatrix_toString);
ZEND_NAMED_FUNCTION(_wrap_new_GEString);
ZEND_NAMED_FUNCTION(_wrap_GEString_getData);
ZEND_NAMED_FUNCTION(_wrap_GEString_setData);
ZEND_NAMED_FUNCTION(_wrap_GEString_size);
ZEND_NAMED_FUNCTION(_wrap_GEString_clear);
ZEND_NAMED_FUNCTION(_wrap_GEString_toString);
ZEND_NAMED_FUNCTION(_wrap_new_GEStringArray);
ZEND_NAMED_FUNCTION(_wrap_GEStringArray_getElement);
ZEND_NAMED_FUNCTION(_wrap_GEStringArray_getData);
ZEND_NAMED_FUNCTION(_wrap_GEStringArray_setData);
ZEND_NAMED_FUNCTION(_wrap_GEStringArray_setElement);
ZEND_NAMED_FUNCTION(_wrap_GEStringArray_toString);
ZEND_NAMED_FUNCTION(_wrap_GEStringArray_size);
ZEND_NAMED_FUNCTION(_wrap_GEStringArray_clear);
ZEND_NAMED_FUNCTION(_wrap_new_GEWorkspace);
ZEND_NAMED_FUNCTION(_wrap_GEWorkspace_setName);
ZEND_NAMED_FUNCTION(_wrap_GEWorkspace_name);
ZEND_NAMED_FUNCTION(_wrap_GEWorkspace_setWorkspace);
ZEND_NAMED_FUNCTION(_wrap_GEWorkspace_workspace);
ZEND_NAMED_FUNCTION(_wrap_GEWorkspace_clear);
ZEND_NAMED_FUNCTION(_wrap_new_WorkspaceManager);
ZEND_NAMED_FUNCTION(_wrap_WorkspaceManager_getCurrent);
ZEND_NAMED_FUNCTION(_wrap_WorkspaceManager_setCurrent);
ZEND_NAMED_FUNCTION(_wrap_WorkspaceManager_getWorkspace);
ZEND_NAMED_FUNCTION(_wrap_WorkspaceManager_destroyAll);
ZEND_NAMED_FUNCTION(_wrap_WorkspaceManager_destroy);
ZEND_NAMED_FUNCTION(_wrap_WorkspaceManager_create);
ZEND_NAMED_FUNCTION(_wrap_WorkspaceManager_workspaceNames);
ZEND_NAMED_FUNCTION(_wrap_WorkspaceManager_count);
ZEND_NAMED_FUNCTION(_wrap_WorkspaceManager_contains);
ZEND_NAMED_FUNCTION(_wrap_WorkspaceManager_isValidWorkspace);
ZEND_NAMED_FUNCTION(_wrap_IGEProgramOutput_invoke);
ZEND_NAMED_FUNCTION(_wrap_new_IGEProgramOutput);
ZEND_NAMED_FUNCTION(_wrap_IGEProgramFlushOutput_invoke);
ZEND_NAMED_FUNCTION(_wrap_new_IGEProgramFlushOutput);
ZEND_NAMED_FUNCTION(_wrap_IGEProgramInputString_invoke);
ZEND_NAMED_FUNCTION(_wrap_IGEProgramInputString_setValue);
ZEND_NAMED_FUNCTION(_wrap_new_IGEProgramInputString);
ZEND_NAMED_FUNCTION(_wrap_IGEProgramInputChar_invoke);
ZEND_NAMED_FUNCTION(_wrap_new_IGEProgramInputChar);
ZEND_NAMED_FUNCTION(_wrap_IGEProgramInputCheck_invoke);
ZEND_NAMED_FUNCTION(_wrap_new_IGEProgramInputCheck);
ZEND_NAMED_FUNCTION(_wrap_new_GESymType);
#endif /* PHP_GAUSS_H */
