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

namespace resolve {
struct winec_t {
  long code{S_OK};
  bool operator()() { return code == S_OK; }
  std::wstring message;
  static winec_t last() {
    winec_t ec;
    ec.code = GetLastError();
    LPWSTR buf = nullptr;
    auto rl = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr,
        ec.code, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPWSTR)&buf, 0,
        nullptr);
    if (rl == 0) {
      ec.message = L"unknown error";
      return ec;
    }
    ec.message.assign(buf, rl);
    LocalFree(buf);
    return ec;
  }
};

std::optional<std::wstring> ResolveLink(std::wstring_view sv);

} // namespace resolve

#endif
