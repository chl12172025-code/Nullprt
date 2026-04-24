#include "editor/EditorWidget.h"

#include <QPlainTextEdit>
#include <QTextCursor>
#include <QVBoxLayout>

namespace nprt::studio {

EditorWidget::EditorWidget(QWidget* parent) : QWidget(parent), fallback_editor_(new QPlainTextEdit(this)) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(fallback_editor_);

  // Scintilla backend hook point:
  // when NPRT_STUDIO_WITH_SCINTILLA is enabled and QScintilla/Scintilla is available,
  // replace fallback_editor_ with the native Scintilla component.
  QObject::connect(fallback_editor_, &QPlainTextEdit::textChanged, [this]() {
    if (text_changed_cb_) {
      text_changed_cb_(fallback_editor_->toPlainText());
    }
  });
}

void EditorWidget::setText(const QString& text) { fallback_editor_->setPlainText(text); }

QString EditorWidget::text() const { return fallback_editor_->toPlainText(); }

int EditorWidget::currentLine() const { return fallback_editor_->textCursor().blockNumber(); }

int EditorWidget::currentCharacter() const { return fallback_editor_->textCursor().positionInBlock(); }

void EditorWidget::setCursorPosition(int line, int character) {
  QTextCursor cursor = fallback_editor_->textCursor();
  cursor.movePosition(QTextCursor::Start);
  for (int i = 0; i < line; ++i) {
    if (!cursor.movePosition(QTextCursor::NextBlock)) break;
  }
  const int col = character < 0 ? 0 : character;
  for (int i = 0; i < col; ++i) {
    if (!cursor.movePosition(QTextCursor::Right)) break;
  }
  fallback_editor_->setTextCursor(cursor);
  fallback_editor_->setFocus();
}

void EditorWidget::insertTextAtCursor(const QString& text) {
  QTextCursor cursor = fallback_editor_->textCursor();
  cursor.insertText(text);
  fallback_editor_->setTextCursor(cursor);
  fallback_editor_->setFocus();
}

void EditorWidget::setTextChangedCallback(std::function<void(const QString&)> cb) {
  text_changed_cb_ = std::move(cb);
}

}  // namespace nprt::studio
