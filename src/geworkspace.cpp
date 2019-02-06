#include "geworkspace.h"
#include "mteng.h"
#include <memory.h>

GEWorkspace::GEWorkspace(WorkspaceHandle_t *wh)
{
    this->workspace_ = wh;
}

GEWorkspace::GEWorkspace(const std::string &name, WorkspaceHandle_t *wh) {
    this->name_ = name;
    this->workspace_ = wh;
}

GEWorkspace::~GEWorkspace() {
    this->clear();
}

void GEWorkspace::setName(const std::string &name) {
    this->name_ = name;
    GAUSS_SetWorkspaceName(this->workspace_, const_cast<char*>(name.data()));
}

std::string GEWorkspace::name() {
    return this->name_;
}

void GEWorkspace::setWorkspace(WorkspaceHandle_t *wh) {
    clear();
    char name[1024];
    memset(&name, 0, sizeof(name));
    GAUSS_GetWorkspaceName(wh, name);
    this->workspace_ = wh;
    this->name_ = std::string(name);
}

WorkspaceHandle_t* GEWorkspace::workspace() {
    return this->workspace_;
}

void GEWorkspace::clear() {
    if (this->workspace_)
        GAUSS_FreeWorkspace(this->workspace_);

    this->workspace_ = 0;
    this->name_.clear();
}
