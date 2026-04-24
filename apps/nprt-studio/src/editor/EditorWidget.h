#pragma once

#include <functional>
#include <QWidget>

class QPlainTextEdit;

namespace nprt::studio {

class EditorWidget : public QWidget {
public:
  explicit EditorWidget(QWidget* parent = nullptr);

  void setText(const QString& text);
  QString text() const;
  int currentLine() const;
  int currentCharacter() const;
  void setCursorPosition(int line, int character);
  void insertTextAtCursor(const QString& text);
  void setTextChangedCallback(std::function<void(const QString&)> cb);

private:
  QPlainTextEdit* fallback_editor_;
  std::function<void(const QString&)> text_changed_cb_;
};

}  // namespace nprt::studio
