///
#include "base.hpp"
#include "reparserpoint.hpp"
#include "resolve.hpp"
#include <winnt.h>
#include <winioctl.h>
#include <ShlObj.h>

namespace resolve {
//
std::optional<std::wstring> ResolveSlink(std::wstring_view sv) {
  if (sv.size() < 4 || sv.size() > MAX_PATH ||
      sv.compare(sv.size() - 4, 4, L".lnk") != 0) {
    return std::nullopt;
  }
  peaz::comptr<IShellLinkW> link;

  if ((CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                        IID_IShellLinkW, (void **)&link)) != S_OK) {
    return std::nullopt;
  }
  peaz::comptr<IPersistFile> pf;
  if (link->QueryInterface(IID_IPersistFile, (void **)&pf) != S_OK) {
    return std::nullopt;
  }
  if (pf->Load(sv.data(), STGM_READ) != S_OK) {
    return std::nullopt;
  }
  if (link->Resolve(nullptr, 0) != S_OK) {
    return std::nullopt;
  }
  WCHAR szPath[MAX_PATH];
  WIN32_FIND_DATA wfd;
  if (link->GetPath(szPath, MAX_PATH, (WIN32_FIND_DATA *)&wfd,
                    SLGP_SHORTPATH) == S_OK) {
    return std::make_optional<std::wstring>(szPath);
  }
  return std::nullopt;
}

std::optional<std::wstring> ResolveLink(std::wstring_view sv,
                                        base::error_code &ec) {
  auto shv = ResolveSlink(sv);
  std::wstring sfile(sv);
  if (shv) {
    sfile = *shv;
  }
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