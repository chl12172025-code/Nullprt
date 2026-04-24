#include "workspace/WorkspaceService.h"

#include <QDir>

namespace nprt::studio {

bool WorkspaceService::openFolder(const QString& path) {
  if (!QDir(path).exists()) {
    return false;
  }
  root_ = path;
  return true;
}

QString WorkspaceService::currentRoot() const { return root_; }

}  // namespace nprt::studio
