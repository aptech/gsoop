#ifndef GAUSS_P_H
#define GAUSS_P_H

#include <string>
#include <cstdlib>
#include <mteng.h>

class WorkspaceManager;
class GEArray;
class GEMatrix;
class GEStringArray;

class GAUSSPrivate
{
public:
    GAUSSPrivate(const std::string &homePath);
    ~GAUSSPrivate();

    std::string gauss_home_;
    WorkspaceManager *manager_;
    static bool managedOutput_;

    Matrix_t* createTempMatrix(GEMatrix*);
    StringArray_t* createTempStringArray(GEStringArray*);
    String_t* createPermString(std::string);
    Array_t* createPermArray(GEArray*);
};

#endif // GAUSS_P_H
