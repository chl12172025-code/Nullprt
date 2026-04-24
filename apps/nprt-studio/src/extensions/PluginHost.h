#pragma once

#include <QString>
#include <QStringList>

namespace nprt::studio {

class PluginHost {
public:
  bool loadPlugin(const QString& pluginPath);
  QStringList loadedPlugins() const;

private:
  QStringList loaded_;
};

}  // namespace nprt::studio
