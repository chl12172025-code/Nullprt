#include "debug/DapClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

namespace nprt::studio {

DapClient::~DapClient() { terminate(); }

bool DapClient::launch(const QString& adapterCommand, const QStringList& args) {
  if (adapterCommand.isEmpty()) return false;
  terminate();

  process_ = new QProcess();
  process_->setProgram(adapterCommand);
  process_->setArguments(args);
  process_->setProcessChannelMode(QProcess::SeparateChannels);
  QObject::connect(process_, &QProcess::readyReadStandardOutput, [this]() { onStdoutReady(); });

  process_->start();
  active_ = process_->waitForStarted(1500);
  if (active_ && output_cb_) output_cb_(QString("DAP started: %1").arg(adapterCommand));
  return active_;
}

bool DapClient::initializeSession() {
  if (!active_) return false;
  QJsonObject args;
  args.insert("adapterID", "nprt-debug");
  args.insert("pathFormat", "path");
  args.insert("linesStartAt1", true);
  args.insert("columnsStartAt1", true);

  QJsonObject req;
  req.insert("seq", seq_++);
  req.insert("type", "request");
  req.insert("command", "initialize");
  req.insert("arguments", args);
  return sendMessage(QJsonDocument(req).toJson(QJsonDocument::Compact));
}

void DapClient::terminate() {
  if (process_) {
    process_->terminate();
    if (!process_->waitForFinished(500)) {
      process_->kill();
      process_->waitForFinished(500);
    }
    delete process_;
    process_ = nullptr;
  }
  active_ = false;
}

bool DapClient::active() const { return active_; }

void DapClient::setOutputCallback(std::function<void(const QString&)> cb) { output_cb_ = std::move(cb); }

bool DapClient::sendMessage(const QByteArray& json) {
  if (!process_ || !active_) return false;
  const QByteArray frame = "Content-Length: " + QByteArray::number(json.size()) + "\r\n\r\n" + json;
  return process_->write(frame) == frame.size();
}

void DapClient::onStdoutReady() {
  if (!process_) return;
  const QString out = QString::fromUtf8(process_->readAllStandardOutput());
  if (!out.isEmpty() && output_cb_) output_cb_(out.trimmed());
}

}  // namespace nprt::studio
