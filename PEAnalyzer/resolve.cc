///
#include "base.hpp"
#include "reparserpoint.hpp"
#include "resolve.hpp"
#include <winnt.h>
#include <winioctl.h>
#include <ShlObj.h>
#include "shl.hpp"
#include "pe.hpp"

namespace resolve {
std::wstring fromascii(std::string_view sv) {
  auto sz = MultiByteToWideChar(CP_ACP, 0, sv.data(), sv.size(), nullptr, 0);
  std::wstring output;
  output.resize(sz);
  // C++17 must output.data()
  MultiByteToWideChar(CP_ACP, 0, sv.data(), sv.size(), output.data(), sz);
  return output;
}
std::wstring ResolveAscii(pecoff::memview mv) {
  std::string s;
  auto it = mv.data();
  auto end = it + mv.size();
  for (; it != end; it++) {
    if (*it == 0) {
      break;
    }
    s.push_back(*it);
  }
  return fromascii(s);
}

std::wstring ResolveUnicode(pecoff::memview mv) {
  std::wstring ws;
  auto it = reinterpret_cast<const wchar_t *>(mv.data());
  auto end = it + mv.size() / 2;
  for (; it != end; it++) {
    if (*it == 0) {
      break;
    }
    ws.push_back(*it);
  }
  return ws;
}

std::optional<std::wstring> ResolveShLink(std::wstring_view sv) {
  if (sv.size() < 4 || sv.compare(sv.size() - 4, 4, L".lnk") != 0) {
    return std::nullopt;
  }
  pecoff::mapview mv;
  auto ec = mv.mapfile(sv, sizeof(shl::shell_link_t), 64 * 1024);
  if (ec) {
    MessageBoxW(nullptr, ec.message.c_str(), L"Dump", MB_OK);
    return std::nullopt;
  }
  auto x = mv.cast<shl::shell_link_t>(0);
  //  typedef struct _GUID {
  //    unsigned long  Data1;
  //    unsigned short Data2;
  //    unsigned short Data3;
  //    unsigned char  Data4[ 8 ];
  //} GUID;
  /// uuid {00021401-0000-0000-C000-000000000046} layout is LE
  constexpr uint8_t shuuid[] = {0x1,  0x14, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0,
                                0xc0, 0x0,  0x0,  0x0, 0x0, 0x0, 0x0, 0x46};
  if (x->dwSize != 0x0000004C ||
      (x->linkflags & shl::HasLinkTargetIDList) == 0 ||
      !ArrayEqual(shuuid, x->uuid)) {
    MessageBoxW(nullptr, L"ss", L"NNNNO", MB_OK);
    return std::nullopt;
  }
  auto pidlist = mv.cast<uint16_t>(x->dwSize);
  if (pidlist == nullptr) {
    return std::nullopt;
  }
  auto offset = *pidlist + x->dwSize + 2;
  if (offset >= mv.size()) {
    return std::nullopt;
  }
  /// resolve LinkInfo
  auto li = mv.cast<shl::link_info_t>(offset);
  if (li == nullptr || (li->dwFlags & shl::VolumeIDAndLocalBasePath) == 0) {
    return std::nullopt;
  }
  std::wstring ws;
  if (li->cbHeaderSize >= 0x00000024) {
    auto xoff = offset + li->cbLocalBasePathOffsetUnicode;
    if (xoff >= mv.size()) {
      return std::nullopt;
    }
    //// Unicode TODO
    ws = ResolveUnicode(pecoff::memview(mv.data() + xoff, mv.size() - xoff));
  } else {
    auto xoff = offset + li->cbLocalBasePathOffset;
    if (xoff >= mv.size()) {
      return std::nullopt;
    }
    ws = ResolveAscii(pecoff::memview(mv.data() + xoff, mv.size() - xoff));
  }
  if (ws.empty()) {
    return std::nullopt;
  }
  return std::make_optional<std::wstring>(ws);
}

std::optional<std::wstring> ResolveLink(std::wstring_view sv,
                                        base::error_code &ec) {
  auto shv = ResolveShLink(sv);
  std::wstring sfile(sv);
  if (shv) {
    sfile = *shv;
  }
#ifndef _M_X64
  FsDisableRedirection fdr;
#endif
  auto hFile = CreateFileW(
      sfile.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      nullptr, OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
  if (hFile == INVALID_HANDLE_VALUE) {
    ec = base::make_system_error_code();
    return std::nullopt;
  }
  char buffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
  DWORD dwBytes = 0;
  if (DeviceIoControl(hFile, FSCTL_GET_REPARSE_POINT, nullptr, 0, buffer,
                      MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &dwBytes,
                      nullptr) != TRUE) {
    CloseHandle(hFile);
    return std::make_optional<std::wstring>(sfile);
  }
  CloseHandle(hFile);

  auto rbuf = reinterpret_cast<REPARSE_DATA_BUFFER *>(buffer);
  switch (rbuf->ReparseTag) {
  case IO_REPARSE_TAG_SYMLINK: {
    auto wstr =
        rbuf->SymbolicLinkReparseBuffer.PathBuffer +
        (rbuf->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR));
    auto wlen =
        rbuf->SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
    if (wlen >= 4 && wstr[0] == L'\\' && wstr[1] == L'?' && wstr[2] == L'?' &&
        wstr[3] == L'\\') {
      /* Starts with \??\ */
      if (wlen >= 6 &&
          ((wstr[4] >= L'A' && wstr[4] <= L'Z') ||
           (wstr[4] >= L'a' && wstr[4] <= L'z')) &&
          wstr[5] == L':' && (wlen == 6 || wstr[6] == L'\\')) {
        /* \??\<drive>:\ */
        wstr += 4;
        wlen -= 4;

      } else if (wlen >= 8 && (wstr[4] == L'U' || wstr[4] == L'u') &&
                 (wstr[5] == L'N' || wstr[5] == L'n') &&
                 (wstr[6] == L'C' || wstr[6] == L'c') && wstr[7] == L'\\') {
        /* \??\UNC\<server>\<share>\ - make sure the final path looks like */
        /* \\<server>\<share>\ */
        wstr += 6;
        wstr[0] = L'\\';
        wlen -= 6;
      }
    }
    return std::make_optional<std::wstring>(wstr, wlen);
  }
  case IO_REPARSE_TAG_MOUNT_POINT: {
    auto wstr =
        rbuf->MountPointReparseBuffer.PathBuffer +
        (rbuf->MountPointReparseBuffer.SubstituteNameOffset / sizeof(WCHAR));
    auto wlen =
        rbuf->MountPointReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
    /* Only treat junctions that look like \??\<drive>:\ as symlink. */
    /* Junctions can also be used as mount points, like \??\Volume{<guid>}, */
    /* but that's confusing for programs since they wouldn't be able to */
    /* actually understand such a path when returned by uv_readlink(). */
    /* UNC paths are never valid for junctions so we don't care about them. */
    if (!(wlen >= 6 && wstr[0] == L'\\' && wstr[1] == L'?' && wstr[2] == L'?' &&
          wstr[3] == L'\\' &&
          ((wstr[4] >= L'A' && wstr[4] <= L'Z') ||
           (wstr[4] >= L'a' && wstr[4] <= L'z')) &&
          wstr[5] == L':' && (wlen == 6 || wstr[6] == L'\\'))) {
      SetLastError(ERROR_SYMLINK_NOT_SUPPORTED);
      ec = base::make_error_code(ERROR_SYMLINK_NOT_SUPPORTED,
                                 L"Symlink not supported");
      return std::nullopt;
    }

    /* Remove leading \??\ */
    wstr += 4;
    wlen -= 4;
    return std::make_optional<std::wstring>(wstr, wlen);
  }
  case IO_REPARSE_TAG_APPEXECLINK: {
    if (rbuf->AppExecLinkReparseBuffer.StringCount != 0) {
      LPWSTR szString = (LPWSTR)rbuf->AppExecLinkReparseBuffer.StringList;
      for (ULONG i = 0; i < rbuf->AppExecLinkReparseBuffer.StringCount; i++) {
        if (i == 2) {
          return std::make_optional<std::wstring>(szString);
        }
        szString += wcslen(szString) + 1;
      }
    }
  } break;
  default:
    break;
  }
  return std::make_optional<std::wstring>(sfile);
}
} // namespace resolve