///
#include "base.hpp"
#include "reparserpoint.hpp"
#include "resolve.hpp"
#include <winnt.h>
#include <winioctl.h>
#include <ShlObj.h>
#include "shl.hpp"
#include "pe.hpp"
#include "endian.hpp"

namespace resolve {
std::wstring fromascii(std::string_view sv) {
  auto sz =
      MultiByteToWideChar(CP_ACP, 0, sv.data(), (int)sv.size(), nullptr, 0);
  std::wstring output;
  output.resize(sz);
  // C++17 must output.data()
  MultiByteToWideChar(CP_ACP, 0, sv.data(), (int)sv.size(), output.data(), sz);
  return output;
}

class shl_memview {
public:
  shl_memview(const char *data__, size_t size__)
      : data_(data__), size_(size__) {
    //
  }
  // --> perpare shl memview
  bool prepare() {
    constexpr uint8_t shuuid[] = {0x1,  0x14, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0,
                                  0xc0, 0x0,  0x0, 0x0, 0x0, 0x0, 0x0, 0x46};
    if (size_ < sizeof(shl::shell_link_t)) {
      return false;
    }
    auto dwSize = planck::readle<uint32_t>(data_);
    if (dwSize != 0x0000004C) {
      return false;
    }
    if (memcmp(data_ + 4, shuuid, ArrayLength(shuuid)) != 0) {
      return false;
    }
    linkflags_ = planck::readle<uint32_t>(data_ + 20);
    IsUnicode = (linkflags_ & shl::IsUnicode) != 0;
    return true;
  }

  const char *data() const { return data_; }
  size_t size() const { return size_; }

  template <typename T> const T *cast(size_t off) const {
    if (off + sizeof(T) >= size_) {
      return nullptr;
    }
    return reinterpret_cast<const T *>(data_ + off);
  }

  uint32_t linkflags() const { return linkflags_; }

  bool stringdata(size_t pos, std::wstring &sd, size_t &sdlen) const {
    if (pos + 2 > size_) {
      return false;
    }
    // CountCharacters (2 bytes): A 16-bit, unsigned integer that specifies
    // either the number of characters, defined by the system default code page,
    // or the number of Unicode characters found in the String field. A value of
    // zero specifies an empty string.
    // String (variable): An optional set of characters, defined by the system
    // default code page, or a Unicode string with a length specified by the
    // CountCharacters field. This string MUST NOT be NULL-terminated.

    auto len = planck::readle<uint16_t>(data_ + pos); /// Ch
    if (IsUnicode) {
      sdlen = len * 2 + 2;
      if (sdlen + pos >= size_) {
        return false;
      }
      auto *p = reinterpret_cast<const uint16_t *>(data_ + pos + 2);
      sd.clear();
      for (size_t i = 0; i < len; i++) {
        // Winodws UTF16LE
        sd.push_back(planck::resolvele(p[i]));
      }
      return true;
    }

    sdlen = len + 2;
    if (sdlen + pos >= size_) {
      return false;
    }
    auto w = fromascii(std::string_view(data_ + pos + 2, len));
    return true;
  }

  bool stringvalue(size_t pos, bool isu, std::wstring &su) {
    if (pos >= size_) {
      return false;
    }
    if (!isu) {
      auto begin = data_ + pos;
      auto end = data_ + size_;
      for (auto it = begin; it != end; it++) {
        if (*it == 0) {
          su = fromascii(std::string_view(begin, it - begin));
          return true;
        }
      }
      return false;
    }
    auto it = (const wchar_t *)(data_ + pos);
    auto end = it + (size_ - pos) / 2;
    for (; it != end; it++) {
      if (*it == 0) {
        return true;
      }
      su.push_back(planck::resolvele(*it));
    }
    return false;
  }

private:
  const char *data_{nullptr};
  size_t size_{0};
  uint32_t linkflags_;
  bool IsUnicode{false};
};

inline std::wstring PathAbsolute(std::wstring_view sv) {
  //::GetFullPathNameW()
  std::wstring ws(0x8000, L'\0');
  auto N = GetFullPathNameW(sv.data(), 0x8000, ws.data(), nullptr);
  if (N > 0 && N < 0x8000) {
    ws.resize(N);
    return ws;
  }
  return L"";
}

struct link_details_t {
  std::wstring target;
  std::wstring relativepath;
};

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
  shl_memview shm(mv.data(), mv.size());
  if (!shm.prepare()) {
    // is not shlink
    return std::nullopt;
  }
  auto flag = shm.linkflags();
  size_t offset = sizeof(shl::shell_link_t);
  if ((flag & shl::HasLinkTargetIDList) != 0) {
    if (shm.size() <= offset + 2) {
      return std::nullopt;
    }
    auto l = planck::readle<uint16_t>(shm.data() + offset);
    if (l + 2 + offset >= shm.size()) {
      return std::nullopt;
    }
    offset += l + 2;
  }
  // LinkINFO https://msdn.microsoft.com/en-us/library/dd871404.aspx

  if ((flag & shl::HasLinkInfo) != 0) {
    auto li = shm.cast<shl::shl_link_infow_t>(offset);
    if (li == nullptr) {
      return std::nullopt;
    }
    auto liflag = planck::resolvele(li->dwFlags);
    if ((liflag & shl::VolumeIDAndLocalBasePath) != 0) {
      bool isunicode;
      size_t pos;
      std::wstring target;
      if (planck::resolvele(li->cbHeaderSize) < 0x00000024) {
        isunicode = false;
        pos = offset + planck::resolvele(li->cbLocalBasePathOffset);
      } else {
        isunicode = true;
        pos = offset + planck::resolvele(li->cbLocalBasePathUnicodeOffset);
      }
      if (!shm.stringvalue(pos, isunicode, target)) {
        return std::nullopt;
      }
      return std::make_optional(target);
    }
    offset += planck::resolvele(li->cbSize);
  }

  std::wstring placeholder;
  size_t sdlen = 0;
  if ((flag & shl::HasName) != 0) {
    if (shm.stringdata(offset, placeholder, sdlen)) {
      offset += sdlen;
    }
  }
  placeholder.clear();
  if ((flag & shl::HasRelativePath) != 0) {
    if (!shm.stringdata(offset, placeholder, sdlen)) {
      return std::nullopt;
    }
    offset += sdlen;
  }
  auto target =
      PathAbsolute(std::wstring(sv).append(L"\\").append(placeholder));
  if (target.empty()) {
    return std::nullopt;
  }
  return std::make_optional(target);
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