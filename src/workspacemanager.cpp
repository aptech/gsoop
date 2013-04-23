#include "workspacemanager.h"
#include "geworkspace.h"
#include <cstring>
using namespace std;

WorkspaceManager::WorkspaceManager()
{
    current_ = 0;
}

GEWorkspace* WorkspaceManager::getCurrent() {
    return current_;
}

bool WorkspaceManager::setCurrent(GEWorkspace *wh) {
    if (!wh)
        return false;

    current_ = wh;

    if (!this->contains(wh)) {
        workspaces_.insert(pair<string, GEWorkspace*>(wh->name(), wh));
    }

    return true;
}

bool WorkspaceManager::contains(GEWorkspace *wh) {
    map<string, GEWorkspace*>::iterator it;

    for (it = workspaces_.begin(); it != workspaces_.end(); ++it)
        if ((*it).second == wh) return true;

    return false;
}

GEWorkspace* WorkspaceManager::getWorkspace(string name) {
    if (workspaces_.find(name) == workspaces_.end())
        return 0;

    return workspaces_[name];
}

void WorkspaceManager::destroyAll() {
    vector<GEWorkspace*> whs;
    map<string, GEWorkspace*>::iterator it;

    if (workspaces_.empty())
        return;

    for (it = workspaces_.begin(); it != workspaces_.end(); ++it)
        whs.push_back((*it).second);

    for (int i = 0; i < whs.size(); ++i)
        destroy(whs.at(i));
}

bool WorkspaceManager::isValidWorkspace(GEWorkspace *wh) {
    return wh && wh->workspace();
}

bool WorkspaceManager::destroy(GEWorkspace *wh) {
    if (current_ == wh)
        current_ = 0;

    if (!wh || !this->contains(wh))
        return false;

    this->workspaces_.erase(workspaces_.find(wh->name()));

    if (wh->workspace() != 0)
        GAUSS_FreeWorkspace(wh->workspace());

    wh->clear();

    delete wh;

    return true;
}

GEWorkspace* WorkspaceManager::create(string name) {
    GEWorkspace *ews = this->getWorkspace(name);

    if (ews)
        return ews;

    GEWorkspace *workspace = new GEWorkspace(name, GAUSS_CreateWorkspace(const_cast<char*>(name.c_str())));

    workspaces_.insert(pair<string, GEWorkspace*>(name, workspace));

    return workspace;
}

vector<string> WorkspaceManager::workspaceNames() {
    vector<string> names;

    names.reserve(this->count());

    map<string, GEWorkspace*>::iterator it;

    for (it = workspaces_.begin(); it != workspaces_.end(); ++it)
        names.push_back((*it).first);

    return names;
}

int WorkspaceManager::count() {
    return workspaces_.size();
}
