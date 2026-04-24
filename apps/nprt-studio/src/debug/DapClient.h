#pragma once

#include <functional>
#include <QString>
#include <QStringList>

class QProcess;

namespace nprt::studio {

class DapClient {
public:
  ~DapClient();

  bool launch(const QString& adapterCommand, const QStringList& args = QStringList());
  bool initializeSession();
  void terminate();
  bool active() const;
  void setOutputCallback(std::function<void(const QString&)> cb);

private:
  bool sendMessage(const QByteArray& json);
  void onStdoutReady();

  QProcess* process_ = nullptr;
  std::function<void(const QString&)> output_cb_;
  int seq_ = 1;
  bool active_ = false;
};

}  // namespace nprt::studio
