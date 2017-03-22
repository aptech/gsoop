#include "workspacemanager.h"
#include "geworkspace.h"
#include <cstring>
using namespace std;

WorkspaceManager::WorkspaceManager() 
	: current_(0)
{
    pthread_mutex_init(&mutex_, NULL);
}

WorkspaceManager::~WorkspaceManager() {
    pthread_mutex_destroy(&mutex_);
}

GEWorkspace* WorkspaceManager::getCurrent() const {
	pthread_mutex_lock(&mutex_);
    GEWorkspace *wh = current_;
    pthread_mutex_unlock(&mutex_);
    return wh;
}

bool WorkspaceManager::setCurrent(GEWorkspace *wh) {
    if (!wh || !wh->workspace())
        return false;

    pthread_mutex_lock(&mutex_);
    current_ = wh;
    pthread_mutex_unlock(&mutex_);

    if (!this->contains(wh)) {
		pthread_mutex_lock(&mutex_);
        workspaces_.insert(pair<string, GEWorkspace*>(wh->name(), wh));
        pthread_mutex_unlock(&mutex_);
    }

    return true;
}

bool WorkspaceManager::contains(GEWorkspace *wh) const {
    unordered_map<string, GEWorkspace*>::const_iterator it;

	pthread_mutex_lock(&mutex_);

    for (it = workspaces_.begin(); it != workspaces_.end(); ++it) {
        if ((*it).second == wh) {
			pthread_mutex_unlock(&mutex_);
            return true;
        }
    }

	pthread_mutex_unlock(&mutex_);

    return false;
}

GEWorkspace* WorkspaceManager::getWorkspace(const string &name) const {
    pthread_mutex_lock(&mutex_);

	if (workspaces_.find(name) == workspaces_.end()) {
		pthread_mutex_unlock(&mutex_);
        return 0;
	}

    GEWorkspace *wksp = workspaces_.at(name);

	pthread_mutex_unlock(&mutex_);

    return wksp;
}

void WorkspaceManager::destroyAll() {
    if (workspaces_.empty())
        return;

    vector<GEWorkspace*> whs;

    pthread_mutex_lock(&mutex_);
    unordered_map<string, GEWorkspace*>::iterator it;
    for (it = workspaces_.begin(); it != workspaces_.end(); ++it)
        whs.push_back((*it).second);
    pthread_mutex_unlock(&mutex_);

    for (unsigned int i = 0; i < whs.size(); ++i)
        destroy(whs.at(i));
}

bool WorkspaceManager::isValidWorkspace(GEWorkspace *wh) const {
    return wh && wh->workspace();
}

bool WorkspaceManager::destroy(GEWorkspace *wh) {
    if (current_ == wh)
        current_ = 0;

    if (!wh || !this->contains(wh))
        return false;

    pthread_mutex_lock(&mutex_);
    this->workspaces_.erase(workspaces_.find(wh->name()));
    pthread_mutex_unlock(&mutex_);

    if (wh->workspace() != 0)
        GAUSS_FreeWorkspace(wh->workspace());

    wh->clear();

    delete wh;

    return true;
}

GEWorkspace* WorkspaceManager::create(const string &name) {
    GEWorkspace *ews = this->getWorkspace(name);

    if (ews)
        return ews;

    GEWorkspace *workspace = new GEWorkspace(name, GAUSS_CreateWorkspace(const_cast<char*>(name.c_str())));

    pthread_mutex_lock(&mutex_);
    workspaces_.insert(pair<string, GEWorkspace*>(name, workspace));
    pthread_mutex_unlock(&mutex_);

    return workspace;
}

vector<string> WorkspaceManager::workspaceNames() const {
    vector<string> names;

    pthread_mutex_lock(&mutex_);

    names.reserve(this->count());

    unordered_map<string, GEWorkspace*>::const_iterator it;

    for (it = workspaces_.begin(); it != workspaces_.end(); ++it)
        names.push_back((*it).first);

    pthread_mutex_unlock(&mutex_);

    return names;
}

int WorkspaceManager::count() const {
    return workspaces_.size();
}
