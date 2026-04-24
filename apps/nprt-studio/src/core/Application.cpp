#include "core/Application.h"

#include <QApplication>

#include "storage/StudioDatabase.h"
#include "ui/MainWindow.h"

namespace nprt::studio {

Application::Application(int& argc, char** argv) {
  qt_app_ = std::make_unique<QApplication>(argc, argv);
  qt_app_->setApplicationName("NPRT Studio");
  qt_app_->setOrganizationName("Nullprt");
}

Application::~Application() = default;

int Application::run() {
  db_ = std::make_unique<StudioDatabase>();
  db_->open();

  main_window_ = std::make_unique<MainWindow>();
  main_window_->show();

  return qt_app_->exec();
}

}  // namespace nprt::studio
