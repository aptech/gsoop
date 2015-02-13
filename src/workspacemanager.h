#ifndef WORKSPACEMANAGER_H
#define WORKSPACEMANAGER_H

#include "gauss.h"
#include <stdio.h>
#include <map>
#include <vector>
#include <iostream>
using namespace std;

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
	pthread_mutex_t mutex_;

    GEWorkspace *current_;
};

#endif // WORKSPACEMANAGER_H
