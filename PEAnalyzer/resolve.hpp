#ifndef PEAZ_RESOLVE_HPP
#define PEAZ_RESOLVE_HPP
#pragma once
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <windows.h>
#endif
#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include "errorcode.hpp"

namespace resolve {
std::optional<std::wstring> ResolveLink(std::wstring_view sv,
                                        base::error_code &ec);

} // namespace resolve

#endif
