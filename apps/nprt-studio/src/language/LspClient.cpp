#include "language/LspClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

namespace nprt::studio {

LspClient::~LspClient() { stop(); }

bool LspClient::start(const QString& command, const QStringList& args) {
  if (command.isEmpty()) {
    return false;
  }
  stop();
  process_ = new QProcess();
  process_->setProgram(command);
  process_->setArguments(args);
  process_->setProcessChannelMode(QProcess::SeparateChannels);

  QObject::connect(process_, &QProcess::readyReadStandardOutput, [this]() { onStdoutReady(); });
  process_->start();
  running_ = process_->waitForStarted(1500);
  return running_;
}

void LspClient::stop() {
  if (process_) {
    process_->closeWriteChannel();
    process_->terminate();
    if (!process_->waitForFinished(500)) {
      process_->kill();
      process_->waitForFinished(500);
    }
    delete process_;
    process_ = nullptr;
  }
  read_buffer_.clear();
  running_ = false;
}

bool LspClient::isRunning() const { return running_; }

bool LspClient::initialize(const QString& rootUri) {
  if (!running_) return false;
  QJsonObject params;
  params.insert("rootUri", rootUri);
  params.insert("processId", 0);
  params.insert("capabilities", QJsonObject());

  QJsonObject req;
  req.insert("jsonrpc", "2.0");
  req.insert("id", next_id_++);
  req.insert("method", "initialize");
  req.insert("params", params);

  return sendMessage(QJsonDocument(req).toJson(QJsonDocument::Compact));
}

bool LspClient::openDocument(const QString& uri, const QString& languageId, const QString& text) {
  if (!running_) return false;
  QJsonObject td;
  td.insert("uri", uri);
  td.insert("languageId", languageId);
  td.insert("version", 1);
  td.insert("text", text);

  QJsonObject params;
  params.insert("textDocument", td);

  QJsonObject req;
  req.insert("jsonrpc", "2.0");
  req.insert("method", "textDocument/didOpen");
  req.insert("params", params);
  return sendMessage(QJsonDocument(req).toJson(QJsonDocument::Compact));
}

bool LspClient::changeDocument(const QString& uri, const QString& text) {
  if (!running_) return false;
  QJsonObject td;
  td.insert("uri", uri);
  td.insert("version", 2);

  QJsonObject change;
  change.insert("text", text);
  QJsonArray contentChanges;
  contentChanges.push_back(change);

  QJsonObject params;
  params.insert("textDocument", td);
  params.insert("contentChanges", contentChanges);

  QJsonObject req;
  req.insert("jsonrpc", "2.0");
  req.insert("method", "textDocument/didChange");
  req.insert("params", params);
  return sendMessage(QJsonDocument(req).toJson(QJsonDocument::Compact));
}

bool LspClient::requestCompletion(const QString& uri, int line, int character) {
  if (!running_) return false;
  QJsonObject params;
  params.insert("uri", uri);
  params.insert("line", line);
  params.insert("character", character);

  QJsonObject req;
  req.insert("jsonrpc", "2.0");
  req.insert("id", next_id_++);
  req.insert("method", "textDocument/completion");
  req.insert("params", params);
  return sendMessage(QJsonDocument(req).toJson(QJsonDocument::Compact));
}

bool LspClient::requestDefinition(const QString& uri, int line, int character) {
  if (!running_) return false;
  QJsonObject params;
  params.insert("uri", uri);
  params.insert("line", line);
  params.insert("character", character);

  QJsonObject req;
  req.insert("jsonrpc", "2.0");
  req.insert("id", next_id_++);
  req.insert("method", "textDocument/definition");
  req.insert("params", params);
  return sendMessage(QJsonDocument(req).toJson(QJsonDocument::Compact));
}

void LspClient::setDiagnosticsCallback(std::function<void(const QString& uri, const QString& message)> cb) {
  diagnostics_cb_ = std::move(cb);
}

void LspClient::setInfoCallback(std::function<void(const QString&)> cb) { info_cb_ = std::move(cb); }

void LspClient::setCompletionCallback(std::function<void(const QStringList&)> cb) {
  completion_cb_ = std::move(cb);
}

void LspClient::setDefinitionCallback(std::function<void(const QString& uri, int line, int character)> cb) {
  definition_cb_ = std::move(cb);
}

bool LspClient::sendMessage(const QByteArray& json) {
  if (!process_ || !running_) return false;
  QByteArray frame = "Content-Length: " + QByteArray::number(json.size()) + "\r\n\r\n" + json;
  return process_->write(frame) == frame.size();
}

QByteArray LspClient::nextMessage() {
  const QByteArray marker("\r\n\r\n");
  const int sep = read_buffer_.indexOf(marker);
  if (sep < 0) return {};

  const QByteArray headers = read_buffer_.left(sep);
  const QList<QByteArray> lines = headers.split('\n');
  int contentLength = -1;
  for (QByteArray line : lines) {
    line = line.trimmed();
    if (line.startsWith("Content-Length:")) {
      contentLength = line.mid(sizeof("Content-Length:") - 1).trimmed().toInt();
      break;
    }
  }
  if (contentLength < 0) return {};

  const int payloadStart = sep + marker.size();
  if (read_buffer_.size() < payloadStart + contentLength) return {};
  const QByteArray payload = read_buffer_.mid(payloadStart, contentLength);
  read_buffer_.remove(0, payloadStart + contentLength);
  return payload;
}

void LspClient::onStdoutReady() {
  read_buffer_.append(process_->readAllStandardOutput());
  while (true) {
    const QByteArray payload = nextMessage();
    if (payload.isEmpty()) break;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
      continue;
    }
    const QJsonObject obj = doc.object();
    if (obj.value("method").toString() == "textDocument/publishDiagnostics") {
      const QJsonObject params = obj.value("params").toObject();
      const QString uri = params.value("uri").toString();
      const QJsonArray diagnostics = params.value("diagnostics").toArray();
      QString message;
      if (!diagnostics.isEmpty()) {
        message = diagnostics.at(0).toObject().value("message").toString();
      }
      if (diagnostics_cb_) diagnostics_cb_(uri, message);
    } else if (obj.contains("id")) {
      const QJsonValue result = obj.value("result");
      if (result.isObject()) {
        const QJsonObject ro = result.toObject();
        if (ro.contains("items")) {
          const QJsonArray items = ro.value("items").toArray();
          const int count = items.size();
          if (completion_cb_) {
            QStringList labels;
            for (const QJsonValue& it : items) {
              labels << it.toObject().value("label").toString();
            }
            completion_cb_(labels);
          }
          if (info_cb_) info_cb_(QString("LSP completion items: %1").arg(count));
        } else if (ro.contains("uri")) {
          const QJsonObject start = ro.value("range").toObject().value("start").toObject();
          const int line = start.value("line").toInt();
          const int character = start.value("character").toInt();
          if (definition_cb_) definition_cb_(ro.value("uri").toString(), line, character);
          if (info_cb_) info_cb_(QString("LSP definition: %1").arg(ro.value("uri").toString()));
        }
      } else if (result.isArray()) {
        if (info_cb_) info_cb_(QString("LSP array result size: %1").arg(result.toArray().size()));
      }
    }
  }
}

}  // namespace nprt::studio
