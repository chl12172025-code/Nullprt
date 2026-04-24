#include "ui/MainWindow.h"

#include <QCoreApplication>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QFileInfo>
#include <QFile>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QAction>
#include <QMenuBar>
#include <QStatusBar>
#include <QTreeView>
#include <QDir>
#include <QUrl>

#include "build/BuildService.h"
#include "debug/DapClient.h"
#include "editor/EditorWidget.h"
#include "extensions/PluginHost.h"
#include "language/LspClient.h"
#include "workspace/WorkspaceService.h"

namespace nprt::studio {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      editor_(new EditorWidget(this)),
      workspace_(new WorkspaceService()),
      build_(new BuildService()),
      lsp_(new LspClient()),
      dap_(new DapClient()),
      plugins_(new PluginHost()),
      problems_list_(nullptr),
      completion_list_(nullptr),
      workspace_model_(nullptr),
      workspace_tree_(nullptr),
      completion_dock_(nullptr),
      current_document_uri_("file:///scratch/main.nprt") {
  setWindowTitle("NPRT Studio");
  resize(1280, 800);

  setCentralWidget(editor_);
  setupMenus();
  setupDockPanels();

  lsp_->setDiagnosticsCallback([this](const QString& uri, const QString& message) {
    if (message.isEmpty()) {
      statusBar()->showMessage("LSP: no diagnostics");
    } else {
      statusBar()->showMessage("LSP diagnostic: " + message);
      appendProblem(uri + " :: " + message);
    }
  });
  lsp_->setInfoCallback([this](const QString& info) {
    if (!info.isEmpty()) {
      statusBar()->showMessage(info);
    }
  });
  lsp_->setCompletionCallback([this](const QStringList& items) { updateCompletionItems(items); });
  lsp_->setDefinitionCallback([this](const QString& uri, int line, int character) {
    if (uri == current_document_uri_) {
      editor_->setCursorPosition(line, character);
    }
  });

  QString lspCmd = "nprt-lsp";
  QStringList lspArgs;
  const QString appDir = QCoreApplication::applicationDirPath();
  const QString localExe = QDir(appDir).filePath("nprt-lsp.exe");
  const QString localBin = QDir::current().filePath("build/windows/nprt-lsp.exe");
  if (QFileInfo::exists(localBin)) {
    lspCmd = localBin;
  } else if (QFileInfo::exists(localExe)) {
    lspCmd = localExe;
  }
  if (lsp_->start(lspCmd, lspArgs)) {
    lsp_->initialize("file:///");
    QString initialText = "fn main() -> i32 {\n  let x: i32 = 1;\n  return x;\n}\n";
    const QString helloPath = QDir::current().filePath("examples/hello_world/src/main.nprt");
    if (QFileInfo::exists(helloPath)) {
      QFile helloFile(helloPath);
      if (helloFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        initialText = QString::fromUtf8(helloFile.readAll());
        current_document_uri_ = QUrl::fromLocalFile(helloPath).toString();
      }
    }
    editor_->setText(initialText);
    lsp_->openDocument(current_document_uri_, "nprt", editor_->text());
    editor_->setTextChangedCallback([this](const QString& text) {
      if (lsp_->isRunning()) {
        lsp_->changeDocument(current_document_uri_, text);
      }
    });
    statusBar()->showMessage("LSP connected");
  } else {
    statusBar()->showMessage("LSP not started");
  }

  build_->setOutputCallback([this](const QString& line) {
    if (!line.isEmpty()) {
      statusBar()->showMessage("Build: " + line.left(120));
    }
  });
  dap_->setOutputCallback([this](const QString& line) {
    if (!line.isEmpty()) {
      statusBar()->showMessage("Debug: " + line.left(120));
    }
  });

}

void MainWindow::setupMenus() {
  QMenu* fileMenu = menuBar()->addMenu("File");
  QAction* openFolderAction = fileMenu->addAction("Open Folder");
  fileMenu->addAction("Save");

  QMenu* runMenu = menuBar()->addMenu("Run");
  QAction* buildAction = runMenu->addAction("Build");
  runMenu->addAction("Run");
  QAction* debugAction = runMenu->addAction("Debug");

  QObject::connect(buildAction, &QAction::triggered, [this]() {
    build_->runAegcBuild("src/self/aegc1/main.nprt", "build/windows/aegc1.exe");
  });
  QObject::connect(debugAction, &QAction::triggered, [this]() {
    QString debugCmd = "nprt-debug";
    const QString localDebug = QDir::current().filePath("build/windows/nprt-debug.exe");
    if (QFileInfo::exists(localDebug)) {
      debugCmd = localDebug;
    }
    if (dap_->launch(debugCmd)) {
      dap_->initializeSession();
      statusBar()->showMessage("Debug adapter connected");
    } else {
      appendProblem("debug :: failed to start nprt-debug adapter");
    }
  });

  QMenu* toolsMenu = menuBar()->addMenu("Tools");
  QAction* paletteAction = toolsMenu->addAction("Command Palette");
  QAction* completionAction = toolsMenu->addAction("LSP Completion At Cursor");
  QAction* definitionAction = toolsMenu->addAction("LSP Definition At Cursor");
  toolsMenu->addAction("Extensions");

  completionAction->setShortcut(QKeySequence("Ctrl+Space"));
  definitionAction->setShortcut(QKeySequence("F12"));
  addAction(completionAction);
  addAction(definitionAction);

  QObject::connect(openFolderAction, &QAction::triggered, [this]() { openWorkspaceFolder(); });
  QObject::connect(paletteAction, &QAction::triggered, [this]() { showCommandPalette(); });
  QObject::connect(completionAction, &QAction::triggered, [this]() {
    if (!lsp_->requestCompletion(current_document_uri_, editor_->currentLine(), editor_->currentCharacter())) {
      appendProblem("lsp :: completion request failed");
    }
  });
  QObject::connect(definitionAction, &QAction::triggered, [this]() {
    if (!lsp_->requestDefinition(current_document_uri_, editor_->currentLine(), editor_->currentCharacter())) {
      appendProblem("lsp :: definition request failed");
    }
  });
}

void MainWindow::setupDockPanels() {
  auto* explorer = new QDockWidget("Explorer", this);
  workspace_model_ = new QFileSystemModel(explorer);
  workspace_model_->setRootPath(QDir::currentPath());
  workspace_tree_ = new QTreeView(explorer);
  workspace_tree_->setModel(workspace_model_);
  workspace_tree_->setRootIndex(workspace_model_->index(QDir::currentPath()));
  explorer->setWidget(workspace_tree_);
  addDockWidget(Qt::LeftDockWidgetArea, explorer);

  auto* problems = new QDockWidget("Problems", this);
  problems_list_ = new QListWidget(problems);
  problems->setWidget(problems_list_);
  addDockWidget(Qt::BottomDockWidgetArea, problems);

  auto* terminal = new QDockWidget("Terminal", this);
  terminal->setWidget(new QLabel("Integrated terminal (placeholder)", terminal));
  addDockWidget(Qt::BottomDockWidgetArea, terminal);

  completion_dock_ = new QDockWidget("Completion", this);
  completion_list_ = new QListWidget(completion_dock_);
  completion_dock_->setWidget(completion_list_);
  addDockWidget(Qt::RightDockWidgetArea, completion_dock_);
  completion_dock_->hide();

  QObject::connect(completion_list_, &QListWidget::itemActivated, [this](QListWidgetItem* item) {
    if (!item) return;
    editor_->insertTextAtCursor(item->text());
    completion_dock_->hide();
  });
}

void MainWindow::appendProblem(const QString& problem) {
  if (!problems_list_) return;
  problems_list_->addItem(problem);
}

void MainWindow::updateCompletionItems(const QStringList& items) {
  if (!completion_list_ || !completion_dock_) return;
  completion_list_->clear();
  completion_list_->addItems(items);
  if (!items.isEmpty()) {
    completion_dock_->show();
    completion_list_->setCurrentRow(0);
  } else {
    completion_dock_->hide();
  }
}

void MainWindow::openWorkspaceFolder() {
  const QString folder = QFileDialog::getExistingDirectory(this, "Open Workspace Folder", QDir::currentPath());
  if (folder.isEmpty()) return;
  if (!workspace_->openFolder(folder)) {
    appendProblem("workspace :: failed to open folder " + folder);
    return;
  }
  if (workspace_model_ && workspace_tree_) {
    workspace_model_->setRootPath(folder);
    workspace_tree_->setRootIndex(workspace_model_->index(folder));
  }
  statusBar()->showMessage("Workspace opened: " + folder);
}

void MainWindow::showCommandPalette() {
  QStringList commands;
  commands << "NPRT: Build"
           << "NPRT: Open Folder"
           << "LSP: Request Completion"
           << "LSP: Go To Definition";

  bool ok = false;
  const QString cmd = QInputDialog::getItem(this, "Command Palette", "Execute command:", commands, 0, false, &ok);
  if (!ok || cmd.isEmpty()) return;

  if (cmd == "NPRT: Build") {
    build_->runAegcBuild("src/self/aegc1/main.nprt", "build/windows/aegc1.exe");
  } else if (cmd == "NPRT: Open Folder") {
    openWorkspaceFolder();
  } else if (cmd == "LSP: Request Completion") {
    if (!lsp_->requestCompletion(current_document_uri_, editor_->currentLine(), editor_->currentCharacter())) {
      appendProblem("lsp :: completion request failed");
    }
  } else if (cmd == "LSP: Go To Definition") {
    if (!lsp_->requestDefinition(current_document_uri_, editor_->currentLine(), editor_->currentCharacter())) {
      appendProblem("lsp :: definition request failed");
    }
  }
}

}  // namespace nprt::studio
