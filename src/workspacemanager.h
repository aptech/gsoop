#ifndef WORKSPACEMANAGER_H
#define WORKSPACEMANAGER_H

#include <stdio.h>
#include <map>
#include <vector>
#include <iostream>
#include "gauss.h"

class WorkspaceManager
{
public:
    WorkspaceManager();

    GEWorkspace* getCurrent();
    bool setCurrent(GEWorkspace *wh);

    GEWorkspace* getWorkspace(std::string);
    void destroyAll();
    bool destroy(GEWorkspace*);
    GEWorkspace* create(std::string);
    std::vector<std::string> workspaceNames();
    int count();
    bool contains(GEWorkspace*);
    bool isValidWorkspace(GEWorkspace*);

private:
    std::map<std::string, GEWorkspace*> workspaces_;

    GEWorkspace *current_;
};

#endif // WORKSPACEMANAGER_H
