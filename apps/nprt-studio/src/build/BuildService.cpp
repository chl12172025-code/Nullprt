#include "build/BuildService.h"

#include <QProcess>

namespace nprt::studio {

BuildService::BuildService() : process_(new QProcess()) {}

BuildService::~BuildService() { delete process_; }

bool BuildService::runAegcBuild(const QString& sourceFile, const QString& outputFile) {
  return runProcess("aegc1", {"-i", sourceFile, "-o", outputFile});
}

bool BuildService::runNprtPkgResolve(const QString& workspaceRoot) {
  return runProcess("nprt-pkg", {"resolve"}, workspaceRoot);
}

void BuildService::setOutputCallback(std::function<void(const QString&)> cb) { output_cb_ = std::move(cb); }

bool BuildService::runProcess(const QString& program, const QStringList& args, const QString& cwd) {
  process_->setProgram(program);
  process_->setArguments(args);
  if (!cwd.isEmpty()) {
    process_->setWorkingDirectory(cwd);
  }
  process_->start();
  if (!process_->waitForStarted(1200)) {
    if (output_cb_) output_cb_(QString("Failed to start process: %1").arg(program));
    return false;
  }
  process_->waitForFinished(10000);
  if (output_cb_) {
    output_cb_(QString::fromUtf8(process_->readAllStandardOutput()));
    const QString err = QString::fromUtf8(process_->readAllStandardError());
    if (!err.isEmpty()) output_cb_(err);
  }
  return process_->exitStatus() == QProcess::NormalExit && process_->exitCode() == 0;
}

}  // namespace nprt::studio
