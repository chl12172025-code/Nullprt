#pragma once

#include <QString>

class QSqlDatabase;

namespace nprt::studio {

class StudioDatabase {
public:
  StudioDatabase();
  ~StudioDatabase();

  bool open();
  QString path() const;

private:
  QString db_path_;
  QSqlDatabase* db_;
};

}  // namespace nprt::studio
