#pragma once

#include <memory>

class QApplication;

namespace nprt::studio {

class MainWindow;
class StudioDatabase;

class Application {
public:
  Application(int& argc, char** argv);
  ~Application();

  int run();

private:
  std::unique_ptr<QApplication> qt_app_;
  std::unique_ptr<StudioDatabase> db_;
  std::unique_ptr<MainWindow> main_window_;
};

}  // namespace nprt::studio
