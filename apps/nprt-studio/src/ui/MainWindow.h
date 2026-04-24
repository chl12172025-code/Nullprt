#pragma once

#include <QMainWindow>
#include <QString>

class QListWidget;
class QFileSystemModel;
class QTreeView;
class QDockWidget;

namespace nprt::studio {

class EditorWidget;
class WorkspaceService;
class BuildService;
class LspClient;
class DapClient;
class PluginHost;

class MainWindow : public QMainWindow {
public:
  explicit MainWindow(QWidget* parent = nullptr);

private:
  void setupMenus();
  void setupDockPanels();
  void appendProblem(const QString& problem);
  void updateCompletionItems(const QStringList& items);
  void openWorkspaceFolder();
  void showCommandPalette();

  EditorWidget* editor_;
  WorkspaceService* workspace_;
  BuildService* build_;
  LspClient* lsp_;
  DapClient* dap_;
  PluginHost* plugins_;
  QListWidget* problems_list_;
  QListWidget* completion_list_;
  QFileSystemModel* workspace_model_;
  QTreeView* workspace_tree_;
  QDockWidget* completion_dock_;
  QString current_document_uri_;
};

}  // namespace nprt::studio
