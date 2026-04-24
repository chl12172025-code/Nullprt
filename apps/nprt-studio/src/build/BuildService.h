#pragma once

#include <functional>
#include <QString>

class QProcess;

namespace nprt::studio {

class BuildService {
public:
  BuildService();
  ~BuildService();

  bool runAegcBuild(const QString& sourceFile, const QString& outputFile);
  bool runNprtPkgResolve(const QString& workspaceRoot);
  void setOutputCallback(std::function<void(const QString&)> cb);

private:
  bool runProcess(const QString& program, const QStringList& args, const QString& cwd = {});
  std::function<void(const QString&)> output_cb_;
  QProcess* process_;
};

}  // namespace nprt::studio
