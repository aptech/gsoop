#include "workspacemanager.h"
#include "geworkspace.h"
#include <cstring>


WorkspaceManager::WorkspaceManager() 
    : current_(0)
{

}

GEWorkspace* WorkspaceManager::getCurrent() const {
    std::lock_guard<std::mutex> guard(mutex_);
    return current_;
}

bool WorkspaceManager::setCurrent(GEWorkspace *wh) {
    if (!wh || !wh->workspace())
        return false;

    mutex_.lock();
    current_ = wh;
    mutex_.unlock();

    if (!this->contains(wh)) {
        std::lock_guard<std::mutex> guard(mutex_);
        workspaces_.insert(std::pair<std::string, GEWorkspace*>(wh->name(), wh));
    }

    return true;
}

bool WorkspaceManager::contains(GEWorkspace *wh) const {
    std::unordered_map<std::string, GEWorkspace*>::const_iterator it;

    std::lock_guard<std::mutex> guard(mutex_);

    for (it = workspaces_.begin(); it != workspaces_.end(); ++it) {
        if (it->second == wh)
            return true;
    }

    return false;
}

GEWorkspace* WorkspaceManager::getWorkspace(const std::string &name) const {
    std::lock_guard<std::mutex> guard(mutex_);

    if (workspaces_.find(name) == workspaces_.end())
        return 0;

    return workspaces_.at(name);
}

void WorkspaceManager::destroyAll() {
    if (workspaces_.empty())
        return;

    std::lock_guard<std::mutex> guard(mutex_);

    std::unordered_map<std::string, GEWorkspace*>::iterator it;
    for (it = workspaces_.begin(); it != workspaces_.end();) {
        GEWorkspace *wh = it->second;
        delete wh;
        it = workspaces_.erase(it);
    }
}

bool WorkspaceManager::isValidWorkspace(GEWorkspace *wh) const {
    return wh && wh->workspace();
}

bool WorkspaceManager::destroy(GEWorkspace *wh) {
    if (current_ == wh)
        current_ = 0;

    if (!wh || !wh->workspace())
        return false;

    mutex_.lock();
    std::unordered_map<std::string, GEWorkspace*>::iterator it = workspaces_.find(wh->name());
    if (it != workspaces_.end())
        this->workspaces_.erase(it);
    mutex_.unlock();

    delete wh;

    return true;
}

GEWorkspace* WorkspaceManager::create(const std::string &name) {
    if (name.empty())
        return nullptr;

    GEWorkspace *ews = this->getWorkspace(name);

    if (ews)
        return ews;

    WorkspaceHandle_t *wh = GAUSS_CreateWorkspace(const_cast<char*>(name.c_str()));

    if (!wh)
        return nullptr;

    GEWorkspace *workspace = new GEWorkspace(name, wh);

    std::lock_guard<std::mutex> guard(mutex_);
    workspaces_.insert(std::pair<std::string, GEWorkspace*>(name, workspace));

    return workspace;
}

std::vector<std::string> WorkspaceManager::workspaceNames() const {
    std::vector<std::string> names;

    std::lock_guard<std::mutex> guard(mutex_);

    names.reserve(workspaces_.size());

    std::unordered_map<std::string, GEWorkspace*>::const_iterator it;

    int count = 0;

    for (it = workspaces_.begin(); it != workspaces_.end(); ++it)
        names.at(count++) = it->first;

    return names;
}

int WorkspaceManager::count() const {
    return workspaces_.size();
}
