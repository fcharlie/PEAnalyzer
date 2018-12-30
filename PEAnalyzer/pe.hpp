#ifndef PEAZ_PE_HPP
#define PEAZ_PE_HPP
#pragma once

#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <windows.h>
#endif
#include <algorithm>
#include <string>
#include <vector>
#include <optional>
#include <string_view>
#include "errorcode.hpp"

#ifndef _M_X64
class FsDisableRedirection {
public:
  typedef BOOL WINAPI fntype_Wow64DisableWow64FsRedirection(PVOID *OldValue);
  typedef BOOL WINAPI fntype_Wow64RevertWow64FsRedirection(PVOID *OldValue);
  FsDisableRedirection() {
    auto pfnWow64DisableWow64FsRedirection =
        (fntype_Wow64DisableWow64FsRedirection *)GetProcAddress(
            GetModuleHandleW(L"kernel32.dll"),
            "Wow64DisableWow64FsRedirection");
    if (pfnWow64DisableWow64FsRedirection) {
      pfnWow64DisableWow64FsRedirection(&OldValue);
    }
  }
  ~FsDisableRedirection() {
    auto pfnWow64RevertWow64FsRedirection =
        (fntype_Wow64RevertWow64FsRedirection *)GetProcAddress(
            GetModuleHandleW(L"kernel32.dll"), "Wow64RevertWow64FsRedirection");
    if (pfnWow64RevertWow64FsRedirection) {
      pfnWow64RevertWow64FsRedirection(&OldValue);
    }
  }

private:
  PVOID OldValue = nullptr;
};
#endif

namespace pecoff {

template <class IntegerT>
inline bool Integer_append_chars(const IntegerT _Raw_value, const int _Base,
                                 std::wstring &wstr) noexcept {
  using _Unsigned = std::make_unsigned_t<IntegerT>;
  _Unsigned _Value = static_cast<_Unsigned>(_Raw_value);
  if constexpr (std::is_signed_v<IntegerT>) {
    if (_Raw_value < 0) {
      wstr.push_back('-');
      _Value = static_cast<_Unsigned>(0 - _Value);
    }
  }

  constexpr size_t _Buff_size =
      sizeof(_Unsigned) * CHAR_BIT; // enough for base 2
  wchar_t _Buff[_Buff_size];
  wchar_t *const _Buff_end = _Buff + _Buff_size;
  wchar_t *_RNext = _Buff_end;

  static constexpr wchar_t _Digits[] = {
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b',
      'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
      'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};
  static_assert(std::size(_Digits) == 36);

  switch (_Base) {
  case 10: { // Derived from _UIntegral_to_buff()
    constexpr bool _Use_chunks = sizeof(_Unsigned) > sizeof(size_t);

    if constexpr (_Use_chunks) { // For 64-bit numbers on 32-bit platforms,
                                 // work in chunks to avoid 64-bit divisions.
      while (_Value > 0xFFFF'FFFFU) {
        unsigned long _Chunk =
            static_cast<unsigned long>(_Value % 1'000'000'000);
        _Value = static_cast<_Unsigned>(_Value / 1'000'000'000);

        for (int _Idx = 0; _Idx != 9; ++_Idx) {
          *--_RNext = static_cast<char>('0' + _Chunk % 10);
          _Chunk /= 10;
        }
      }
    }

    using _Truncated =
        std::conditional_t<_Use_chunks, unsigned long, _Unsigned>;

    _Truncated _Trunc = static_cast<_Truncated>(_Value);

    do {
      *--_RNext = static_cast<wchar_t>('0' + _Trunc % 10);
      _Trunc /= 10;
    } while (_Trunc != 0);
    break;
  }

  case 2:
    do {
      *--_RNext = static_cast<wchar_t>('0' + (_Value & 0b1));
      _Value >>= 1;
    } while (_Value != 0);
    break;

  case 4:
    do {
      *--_RNext = static_cast<wchar_t>('0' + (_Value & 0b11));
      _Value >>= 2;
    } while (_Value != 0);
    break;

  case 8:
    do {
      *--_RNext = static_cast<wchar_t>('0' + (_Value & 0b111));
      _Value >>= 3;
    } while (_Value != 0);
    break;

  case 16:
    do {
      *--_RNext = _Digits[_Value & 0b1111];
      _Value >>= 4;
    } while (_Value != 0);
    break;

  case 32:
    do {
      *--_RNext = _Digits[_Value & 0b11111];
      _Value >>= 5;
    } while (_Value != 0);
    break;

  default:
    do {
      *--_RNext = _Digits[_Value % _Base];
      _Value = static_cast<_Unsigned>(_Value / _Base);
    } while (_Value != 0);
    break;
  }
  const ptrdiff_t _Digits_written = _Buff_end - _RNext;
  wstr.append(_RNext, _Digits_written);
  return true;
}

template <class IntegerT>
[[nodiscard]] inline std::wstring
Integer_to_chars(const IntegerT _Raw_value,
                 const int _Base) noexcept // strengthened
{
  std::wstring wr;
  Integer_append_chars(_Raw_value, _Base, wr);
  return wr;
}

class memview {
public:
  static constexpr std::size_t npos = SIZE_MAX;
  memview() = default;
  memview(const char *d, std::size_t l) : data_(d), size_(l) {}
  memview(const memview &other) {
    data_ = other.data_;
    size_ = other.size_;
  }

  template <size_t ArrayLen> bool startswith(const uint8_t (&bv)[ArrayLen]) {
    return (size_ >= ArrayLen && memcmp(data_, bv, ArrayLen) == 0);
  }

  bool startswith(std::string_view sv) {
    if (sv.size() > size_) {
      return false;
    }
    return (memcmp(data_, sv.data(), sv.size()) == 0);
  }

  bool startswith(const void *p, size_t n) {
    if (n > size_) {
      return false;
    }
    return (memcmp(data_, p, n) == 0);
  }

  bool startswith(memview mv) {
    if (mv.size_ > size_) {
      return false;
    }
    return (memcmp(data_, mv.data_, mv.size_) == 0);
  }
  bool indexswith(std::size_t offset, std::string_view sv) const {
    if (offset > size_) {
      return false;
    }
    return memcmp(data_ + offset, sv.data(), sv.size()) == 0;
  }
  memview submv(std::size_t pos, std::size_t n = npos) {
    return memview(data_ + pos, (std::min)(n, size_ - pos));
  }
  std::size_t size() const { return size_; }
  const char *data() const { return data_; }
  std::string_view sv() { return std::string_view(data_, size_); }
  unsigned char operator[](const std::size_t off) const {
    if (off >= size_) {
      return UCHAR_MAX;
    }
    return (unsigned char)data_[off];
  }
  template <typename T> const T *cast(size_t off) {
    if (off + sizeof(T) >= size_) {
      return nullptr;
    }
    return reinterpret_cast<const T *>(data_ + off);
  }

private:
  const char *data_{nullptr};
  std::size_t size_{0};
};

class mapview {
public:
  static constexpr auto nullfile_t = INVALID_HANDLE_VALUE;
  static void Close(HANDLE handle) {
    if (handle != nullfile_t) {
      CloseHandle(handle);
    }
  }

  mapview() = default;
  mapview(const mapview &) = delete;
  mapview &operator=(const mapview &) = delete;
  ~mapview() {
    if (data_ != nullptr) {
      ::UnmapViewOfFile(data_);
    }
    Close(FileMap);
    Close(FileHandle);
  }
  base::error_code mapfile(std::wstring_view file, std::size_t minsize = 1,
                           std::size_t maxsize = SIZE_MAX) {
#ifndef _M_X64
    FsDisableRedirection fdr;
#endif
    if ((FileHandle = CreateFileW(file.data(), GENERIC_READ,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                  nullptr)) == nullfile_t) {
      return base::make_system_error_code();
    }
    LARGE_INTEGER li;
    if (GetFileSizeEx(FileHandle, &li) != TRUE) {
      return base::make_system_error_code();
    }
    if ((std::size_t)li.QuadPart < minsize) {
      return base::make_error_code(L"filesize too small");
    }
    if ((FileMap = CreateFileMappingW(FileHandle, nullptr, PAGE_READONLY, 0, 0,
                                      nullptr)) == nullfile_t) {
      return base::make_system_error_code();
      ;
    }
    size_ = (size_t)li.QuadPart > maxsize ? maxsize : (size_t)li.QuadPart;
    auto baseAddr = MapViewOfFile(FileMap, FILE_MAP_READ, 0, 0, size_);
    if (baseAddr == nullptr) {
      return base::make_system_error_code();
      ;
    }
    data_ = reinterpret_cast<char *>(baseAddr);
    return base::error_code{};
  }
  std::size_t size() const { return size_; }
  const char *data() const { return data_; }
  unsigned char operator[](const std::size_t off) const {
    if (off >= size_) {
      return 255;
    }
    return (unsigned char)data_[off];
  }
  bool startswith(const char *prefix, size_t pl) const {
    if (pl >= size_) {
      return false;
    }
    return memcmp(data_, prefix, pl) == 0;
  }
  bool startswith(std::string_view sv) const {
    return startswith(sv.data(), sv.size());
  }
  bool indexswith(std::size_t offset, std::string_view sv) const {
    if (offset > size_) {
      return false;
    }
    return memcmp(data_ + offset, sv.data(), sv.size()) == 0;
  }

  template <typename T> const T *cast(size_t off) {
    if (off + sizeof(T) >= size_) {
      return nullptr;
    }
    return reinterpret_cast<const T *>(data_ + off);
  }

  memview subview(size_t off) {
    if (off >= size_) {
      return memview();
    }
    return memview(data_ + off, size_ - off);
  }

private:
  HANDLE FileHandle{nullfile_t};
  HANDLE FileMap{nullfile_t};
  char *data_{nullptr};
  std::size_t size_{0};
};

struct pe_version_t {
  uint16_t major{0};
  uint16_t minor{0};
  std::wstring strversion() {
    auto sver = Integer_to_chars(major, 10).append(L".");
    Integer_append_chars(minor, 10, sver);
    return sver;
  }
};

struct pe_minutiae_t {
  std::wstring machine;
  std::wstring subsystem;
  std::wstring clrmsg;
  std::vector<std::wstring> characteristics;
  std::vector<std::wstring> depends; /// DLL required
  pe_version_t osver;
  pe_version_t linkver;
  pe_version_t imagever;
  bool isdll;
};
std::optional<pe_minutiae_t> inquisitive_pecoff(std::wstring_view sv,
                                                base::error_code &ec);
} // namespace pecoff

#endif
