#pragma once

#include <QIcon>
#include <QString>

namespace GameFinder {

[[nodiscard]] unsigned int getDefaultGameAppId();

[[nodiscard]] QString getGameInstallPath(unsigned int appId);

[[nodiscard]] QIcon getGameIcon(unsigned int appId);

} // namespace GameFinder
