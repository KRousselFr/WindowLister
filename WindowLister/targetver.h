#pragma once

// Si vous incluez SDKDDKVer.h, cela définit la dernière plateforme Windows disponible.

// Si vous souhaitez générer votre application pour une plateforme Windows précédente, incluez WinSDKVer.h et
// définissez la macro _WIN32_WINNT à la plateforme que vous souhaitez prendre en charge avant d'inclure SDKDDKVer.h.

#include <winsdkver.h>
#define _WIN32_WINNT  _WIN32_WINNT_NT4
#include <SDKDDKVer.h>

