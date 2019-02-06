#ifndef GEWORKSPACE_H
#define GEWORKSPACE_H

#include <stdio.h>
#include "gauss.h"
#include <cstdlib>
#include <string>

/**
  * Wrapper for a WorkspaceHandle_t* object.
  *
  */
class GAUSS_EXPORT GEWorkspace
{
public:
    GEWorkspace(WorkspaceHandle_t*);
    GEWorkspace(const std::string &, WorkspaceHandle_t*);
    ~GEWorkspace();

    void setName(const std::string &);
    std::string name();

    void setWorkspace(WorkspaceHandle_t*);
    WorkspaceHandle_t* workspace();

    void clear();

private:
    std::string name_;
    WorkspaceHandle_t *workspace_;
};

#endif // GEWORKSPACE_H
