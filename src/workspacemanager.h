#ifndef WORKSPACEMANAGER_H
#define WORKSPACEMANAGER_H

#include "gauss.h"
#include <stdio.h>
#include <map>
#include <vector>
#include <iostream>
using namespace std;

class GAUSS_EXPORT WorkspaceManager
{
public:
    WorkspaceManager();
    ~WorkspaceManager();

    GEWorkspace* getCurrent() const;
    bool setCurrent(GEWorkspace *wh);

    GEWorkspace* getWorkspace(const std::string &) const;
    void destroyAll();
    bool destroy(GEWorkspace*);
    GEWorkspace* create(const std::string &);
    std::vector<std::string> workspaceNames() const;
    int count() const;
    bool contains(GEWorkspace*) const;
    bool isValidWorkspace(GEWorkspace*) const;

private:
    std::map<std::string, GEWorkspace*> workspaces_;
    mutable pthread_mutex_t mutex_;

    GEWorkspace *current_;
};

#endif // WORKSPACEMANAGER_H
