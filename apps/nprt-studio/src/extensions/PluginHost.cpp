#include "extensions/PluginHost.h"

namespace nprt::studio {

bool PluginHost::loadPlugin(const QString& pluginPath) {
  if (pluginPath.isEmpty()) {
    return false;
  }
  loaded_.append(pluginPath);
  return true;
}

QStringList PluginHost::loadedPlugins() const { return loaded_; }

}  // namespace nprt::studio
