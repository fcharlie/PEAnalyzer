/////////
#ifndef PEAZ_CHARCONV_HPP
#define PEAZ_CHARCONV_HPP
#include <string>
#include <climits>

namespace base {

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

} // namespace base

#endif