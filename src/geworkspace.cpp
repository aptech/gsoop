#include "geworkspace.h"
#include "mteng.h"

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
}

std::string GEWorkspace::name() {
    return this->name_;
}

void GEWorkspace::setWorkspace(WorkspaceHandle_t *wh) {
    this->workspace_ = wh;
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
