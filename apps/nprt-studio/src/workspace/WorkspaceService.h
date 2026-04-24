#pragma once

#include <QString>

namespace nprt::studio {

class WorkspaceService {
public:
  bool openFolder(const QString& path);
  QString currentRoot() const;

private:
  QString root_;
};

}  // namespace nprt::studio
