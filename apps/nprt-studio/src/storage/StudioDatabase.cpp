#include "storage/StudioDatabase.h"

#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>

namespace nprt::studio {

StudioDatabase::StudioDatabase() : db_(nullptr) {}

StudioDatabase::~StudioDatabase() {
  if (db_) {
    db_->close();
  }
}

bool StudioDatabase::open() {
  const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir().mkpath(appData);
  db_path_ = appData + "/studio.db";

  if (!QSqlDatabase::contains("nprt_studio")) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "nprt_studio");
    db.setDatabaseName(db_path_);
    db_ = new QSqlDatabase(db);
  } else {
    QSqlDatabase db = QSqlDatabase::database("nprt_studio");
    db_ = new QSqlDatabase(db);
  }

  if (!db_->open()) {
    return false;
  }

  QSqlQuery q(*db_);
  q.exec("CREATE TABLE IF NOT EXISTS recent_workspace(path TEXT PRIMARY KEY, opened_at TEXT)");
  return true;
}

QString StudioDatabase::path() const { return db_path_; }

}  // namespace nprt::studio
