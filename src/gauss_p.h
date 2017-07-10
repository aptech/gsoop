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

    StringArray_t* createPermStringArray(GEStringArray*);
    String_t* createPermString(const std::string &);
};

#endif // GAUSS_P_H
