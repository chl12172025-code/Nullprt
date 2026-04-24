#pragma once

#include <functional>
#include <QString>
#include <QStringList>

class QProcess;

namespace nprt::studio {

class LspClient {
public:
  ~LspClient();

  bool start(const QString& command, const QStringList& args = QStringList());
  void stop();
  bool isRunning() const;
  bool initialize(const QString& rootUri);
  bool openDocument(const QString& uri, const QString& languageId, const QString& text);
  bool changeDocument(const QString& uri, const QString& text);
  bool requestCompletion(const QString& uri, int line, int character);
  bool requestDefinition(const QString& uri, int line, int character);

  void setDiagnosticsCallback(std::function<void(const QString& uri, const QString& message)> cb);
  void setInfoCallback(std::function<void(const QString&)> cb);
  void setCompletionCallback(std::function<void(const QStringList&)> cb);
  void setDefinitionCallback(std::function<void(const QString& uri, int line, int character)> cb);

private:
  bool sendMessage(const QByteArray& json);
  QByteArray nextMessage();
  void onStdoutReady();

  QProcess* process_ = nullptr;
  QByteArray read_buffer_;
  int next_id_ = 1;
  bool running_ = false;
  std::function<void(const QString& uri, const QString& message)> diagnostics_cb_;
  std::function<void(const QString&)> info_cb_;
  std::function<void(const QStringList&)> completion_cb_;
  std::function<void(const QString& uri, int line, int character)> definition_cb_;
};

}  // namespace nprt::studio
