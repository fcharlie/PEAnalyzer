#ifndef ERRORCODE_HPP
#define ERRORCODE_HPP
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

namespace base {
struct error_code {
  std::wstring message;
  long code{NO_ERROR};
  explicit operator bool() const noexcept { return code != NO_ERROR; }
};
inline error_code make_error_code(int val, std::wstring_view msg) {
  return error_code{std::wstring(msg), val};
}
inline error_code make_error_code(std::wstring_view msg) {
  return error_code{std::wstring(msg), -1};
}
inline error_code make_system_error_code() {
  error_code ec;
  ec.code = GetLastError();
  LPWSTR buf = nullptr;
  auto rl = FormatMessageW(
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr,
      ec.code, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPWSTR)&buf, 0,
      nullptr);
  if (rl == 0) {
    ec.message = L"FormatMessageW error";
    return ec;
  }
  ec.message.assign(buf, rl);
  LocalFree(buf);
  return ec;
}
} // namespace base

#endif