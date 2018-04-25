/**
 **/

#include "stdafx.h"
#include <Dbghelp.h>
#include <string>
#include <string_view>
#include <winnt.h>
#include <winioctl.h>
///
#include "PortableExecutableFile.h"
#pragma comment(lib, "DbgHelp.lib")

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

inline std::wstring to_utf16(const std::string_view &s) {
  const int size = MultiByteToWideChar(CP_UTF8, 0, s.data(), -1, nullptr, 0);
  std::wstring output;
  output.resize(size - 1);
  MultiByteToWideChar(CP_UTF8, 0, s.data(), -1, output.data(), size - 1);
  return output;
}

class ErrorMessage {
public:
  ErrorMessage(DWORD errid) : lastError(errid) {
    if (FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            nullptr, errid, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
            (LPWSTR)&buf, 0, nullptr) == 0) {
      buf = nullptr;
    }
  }
  ~ErrorMessage() {
    if (buf != nullptr) {
      LocalFree(buf);
    }
  }
  const wchar_t *message() const { return buf == nullptr ? L"unknwon" : buf; }
  DWORD LastError() const { return lastError; }

private:
  DWORD lastError;
  LPWSTR buf{nullptr};
};

struct ReparseBuffer {
  ReparseBuffer() {
    data = reinterpret_cast<REPARSE_DATA_BUFFER *>(
        malloc(MAXIMUM_REPARSE_DATA_BUFFER_SIZE));
  }
  ~ReparseBuffer() {
    if (data != nullptr) {
      free(data);
    }
  }
  REPARSE_DATA_BUFFER *data{nullptr};
};

bool Readlink(const std::wstring &symfile, std::wstring &realfile) {
  auto hFile = CreateFileW(
      symfile.c_str(), 0,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
      NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    ErrorMessage err(GetLastError());
    fwprintf(stderr, L"CreateFileW: %s\n", err.message());
    return false;
  }
  ReparseBuffer rbuf;
  DWORD dwBytes = 0;
  if (DeviceIoControl(hFile, FSCTL_GET_REPARSE_POINT, nullptr, 0, rbuf.data,
                      MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &dwBytes,
                      nullptr) != TRUE) {
    ErrorMessage err(GetLastError());
    fwprintf(stderr, L"DeviceIoControl: %s\n", err.message());
    CloseHandle(hFile);
    return false;
  }
  CloseHandle(hFile);
  switch (rbuf.data->ReparseTag) {
  case IO_REPARSE_TAG_SYMLINK: {
    auto wstr = rbuf.data->SymbolicLinkReparseBuffer.PathBuffer +
                (rbuf.data->SymbolicLinkReparseBuffer.SubstituteNameOffset /
                 sizeof(WCHAR));
    auto wlen = rbuf.data->SymbolicLinkReparseBuffer.SubstituteNameLength /
                sizeof(WCHAR);
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
    realfile.assign(wstr, wlen);
  } break;
  case IO_REPARSE_TAG_MOUNT_POINT: {
    auto wstr = rbuf.data->MountPointReparseBuffer.PathBuffer +
                (rbuf.data->MountPointReparseBuffer.SubstituteNameOffset /
                 sizeof(WCHAR));
    auto wlen =
        rbuf.data->MountPointReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
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
      return false;
    }

    /* Remove leading \??\ */
    wstr += 4;
    wlen -= 4;
    realfile.assign(wstr, wlen);
  } break;
  case IO_REPARSE_TAG_APPEXECLINK: {
    if (rbuf.data->AppExecLinkReparseBuffer.StringCount != 0) {
      LPWSTR szString = (LPWSTR)rbuf.data->AppExecLinkReparseBuffer.StringList;
      for (ULONG i = 0; i < rbuf.data->AppExecLinkReparseBuffer.StringCount;
           i++) {
        if (i == 2) {
          realfile = szString;
        }
        szString += wcslen(szString) + 1;
      }
    }
  } break;
  default:
    return false;
  }
  return true;
}

struct VALUE_STRING {
  int index;
  const wchar_t *str;
};

const VALUE_STRING archList[] = {
    {PROCESSOR_ARCHITECTURE_INTEL, L"Win32"},
    {PROCESSOR_ARCHITECTURE_MIPS, L"MIPS"},
    {PROCESSOR_ARCHITECTURE_ALPHA, L"Alpha"},
    {PROCESSOR_ARCHITECTURE_PPC, L"PPC"},
    {PROCESSOR_ARCHITECTURE_SHX, L"SHX"},
    {PROCESSOR_ARCHITECTURE_ARM, L"ARM"},
    {PROCESSOR_ARCHITECTURE_IA64, L"IA64"},
    {PROCESSOR_ARCHITECTURE_ALPHA64, L"Alpha64"},
    {PROCESSOR_ARCHITECTURE_MSIL, L"MSIL"},
    {PROCESSOR_ARCHITECTURE_AMD64, L"Win64"},
    {PROCESSOR_ARCHITECTURE_IA32_ON_WIN64, L"Wow64"},
    {PROCESSOR_ARCHITECTURE_NEUTRAL, L"Neutral"},
    {PROCESSOR_ARCHITECTURE_ARM64, L"ARM64"},
    {PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64, L"ARM32-Win64"},
    {PROCESSOR_ARCHITECTURE_IA32_ON_ARM64, L"IA32-ARM64"},
};

const wchar_t *ArchitectureName(int id) {
  for (auto &x : archList) {
    if (x.index == id)
      return x.str;
  }
  return L"Unknown";
}

const VALUE_STRING machineTable[] = {
    {IMAGE_FILE_MACHINE_UNKNOWN, L"Unknown Machine"},
    {IMAGE_FILE_MACHINE_TARGET_HOST, L"WoW Gest"},
    {IMAGE_FILE_MACHINE_I386, L"Intel 386"},
    {IMAGE_FILE_MACHINE_R3000, L"MIPS little-endian, 0x160 big-endian"},
    {IMAGE_FILE_MACHINE_R4000, L"MIPS little-endian"},
    {IMAGE_FILE_MACHINE_R10000, L"MIPS little-endian"},
    {IMAGE_FILE_MACHINE_WCEMIPSV2, L"MIPS little-endian WCE v2"},
    {IMAGE_FILE_MACHINE_ALPHA, L"Alpha_AXP"},
    {IMAGE_FILE_MACHINE_SH3, L"SH3 little-endian"},
    {IMAGE_FILE_MACHINE_SH3DSP, L"SH3 DSP"},
    {IMAGE_FILE_MACHINE_SH3E, L"SH3E little-endian"},
    {IMAGE_FILE_MACHINE_SH4, L"SH4 little-endian"},
    {IMAGE_FILE_MACHINE_SH5, L"SH5"},
    {IMAGE_FILE_MACHINE_ARM, L"ARM Little-Endian"},
    {IMAGE_FILE_MACHINE_THUMB, L"ARM Thumb/Thumb-2 Little-Endian"},
    {IMAGE_FILE_MACHINE_ARMNT, L"ARM Thumb-2 Little-Endian"},
    {IMAGE_FILE_MACHINE_AM33, L"Matsushita AM33"},
    {IMAGE_FILE_MACHINE_POWERPC, L"IBM PowerPC Little-Endian"},
    {IMAGE_FILE_MACHINE_POWERPCFP, L"IBM PowerPC  (FP support)"},
    {IMAGE_FILE_MACHINE_IA64, L"Intel IA64"},
    {IMAGE_FILE_MACHINE_MIPS16, L"MIPS"},
    {IMAGE_FILE_MACHINE_ALPHA64, L"ALPHA64"},
    {IMAGE_FILE_MACHINE_MIPSFPU, L"MIPS"},
    {IMAGE_FILE_MACHINE_MIPSFPU16, L"MIPS"},
    {IMAGE_FILE_MACHINE_TRICORE, L"Infineon"},
    {IMAGE_FILE_MACHINE_CEF, L"IMAGE_FILE_MACHINE_CEF"},
    {IMAGE_FILE_MACHINE_EBC, L"EFI Byte Code"},
    {IMAGE_FILE_MACHINE_AMD64, L"AMD64 (K8)"},
    {IMAGE_FILE_MACHINE_M32R, L"M32R little-endian"},
    {IMAGE_FILE_MACHINE_ARM64, L"ARM64 Little-Endian"},
    {IMAGE_FILE_MACHINE_CEE, L"IMAGE_FILE_MACHINE_CEE"}};

const VALUE_STRING subsystemTable[] = {
    {IMAGE_SUBSYSTEM_UNKNOWN, L"Unknown subsystem"},
    {IMAGE_SUBSYSTEM_NATIVE, L"Native"}, // not require subsystem
    {IMAGE_SUBSYSTEM_WINDOWS_GUI, L"Windows GUI"},
    {IMAGE_SUBSYSTEM_WINDOWS_CUI, L"Windows CUI"},
    {IMAGE_SUBSYSTEM_OS2_CUI, L"OS/2  CUI"},
    {IMAGE_SUBSYSTEM_POSIX_CUI, L"Posix character subsystem"},
    {IMAGE_SUBSYSTEM_NATIVE_WINDOWS, L"Native Win9x driver"},
    {IMAGE_SUBSYSTEM_WINDOWS_CE_GUI, L"Windows CE subsystem"},
    {IMAGE_SUBSYSTEM_EFI_APPLICATION, L"EFI Application"},
    {IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER, L"EFI Boot Service Driver"},
    {IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER, L"EFI Runtime Driver"},
    {IMAGE_SUBSYSTEM_EFI_ROM, L"EFI ROM"},
    {IMAGE_SUBSYSTEM_XBOX, L"Xbox system"},
    {IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION, L"Windows Boot Application"},
    {IMAGE_SUBSYSTEM_XBOX_CODE_CATALOG, L"XBOX Code Catalog"}};

PortableExecutableFile::PortableExecutableFile(const std::wstring &mPath)
    : mPath_(mPath) {
  subsystem = subsystemTable[0].str;
  clrMessage = L"Native Executable File";
}
/*
typedef enum _SID_NAME_USE {
SidTypeUser = 1,
SidTypeGroup,
SidTypeDomain,
SidTypeAlias,
SidTypeWellKnownGroup,
SidTypeDeletedAccount,
SidTypeInvalid,
SidTypeUnknown,
SidTypeComputer,
SidTypeLabel,
SidTypeLogonSession
} SID_NAME_USE, *PSID_NAME_USE;
*/

/*
//
// ARM64 relocations types.
//

#define IMAGE_REL_ARM64_ABSOLUTE        0x0000  // No relocation required
#define IMAGE_REL_ARM64_ADDR32          0x0001  // 32 bit address. Review! do we
need it? #define IMAGE_REL_ARM64_ADDR32NB        0x0002  // 32 bit address w/o
image base (RVA: for Data/PData/XData) #define IMAGE_REL_ARM64_BRANCH26
0x0003  // 26 bit offset << 2 & sign ext. for B & BL #define
IMAGE_REL_ARM64_PAGEBASE_REL21  0x0004  // ADRP #define IMAGE_REL_ARM64_REL21
0x0005  // ADR #define IMAGE_REL_ARM64_PAGEOFFSET_12A  0x0006  // ADD/ADDS
(immediate) with zero shift, for page offset #define
IMAGE_REL_ARM64_PAGEOFFSET_12L  0x0007  // LDR (indexed, unsigned immediate),
for page offset #define IMAGE_REL_ARM64_SECREL          0x0008  // Offset within
section #define IMAGE_REL_ARM64_SECREL_LOW12A   0x0009  // ADD/ADDS (immediate)
with zero shift, for bit 0:11 of section offset #define
IMAGE_REL_ARM64_SECREL_HIGH12A  0x000A  // ADD/ADDS (immediate) with zero shift,
for bit 12:23 of section offset #define IMAGE_REL_ARM64_SECREL_LOW12L   0x000B
// LDR (indexed, unsigned immediate), for bit 0:11 of section offset #define
IMAGE_REL_ARM64_TOKEN           0x000C #define IMAGE_REL_ARM64_SECTION
0x000D  // Section table index #define IMAGE_REL_ARM64_ADDR64          0x000E
// 64 bit address

//
*/

/*
// Code Integrity in loadconfig (CI)
//

typedef struct _IMAGE_LOAD_CONFIG_CODE_INTEGRITY {
WORD    Flags;          // Flags to indicate if CI information is available,
etc. WORD    Catalog;        // 0xFFFF means not available DWORD
CatalogOffset; DWORD   Reserved;       // Additional bitmask to be defined later
} IMAGE_LOAD_CONFIG_CODE_INTEGRITY, *PIMAGE_LOAD_CONFIG_CODE_INTEGRITY;

//
*/

bool PortableExecutableFile::AnalyzePE64(PIMAGE_NT_HEADERS64 nh, void *hd) {
  auto oh = reinterpret_cast<IMAGE_OPTIONAL_HEADER64 *>(hd);
  for (auto &sub : subsystemTable) {
    if (sub.index == oh->Subsystem) {
      subsystem = sub.str;
    }
  }
  wsprintfW(linkVersion, L"%d.%d", oh->MajorLinkerVersion,
            oh->MinorLinkerVersion);
  wsprintfW(osVersion, L"%d.%d", oh->MajorOperatingSystemVersion,
            oh->MinorOperatingSystemVersion);
  wsprintfW(subsystemVersion, L"%d.%d", oh->MajorSubsystemVersion,
            oh->MinorSubsystemVersion);
  wsprintfW(imageVersion, L"%d.%d", oh->MajorImageVersion,
            oh->MinorImageVersion);
  auto entry = &(oh->DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER]);

  if (entry->Size != sizeof(IMAGE_COR20_HEADER)) {
    return true;
  }
  auto baseAddr = mv.BaseAddress();
  auto va =
      ImageRvaToVa((PIMAGE_NT_HEADERS)nh, baseAddr, entry->VirtualAddress, 0);
  if ((char *)va - baseAddr > mv.FileSize()) {
    return false;
  }
  auto *clr = reinterpret_cast<IMAGE_COR20_HEADER *>(va);
  auto pm = reinterpret_cast<char *>(ImageRvaToVa(
      (PIMAGE_NT_HEADERS)nh, baseAddr, clr->MetaData.VirtualAddress, 0));
  if ((char *)pm - baseAddr > mv.FileSize()) {
    return false;
  }
  auto buildMessage = pm + 16;
  clrMessage = to_utf16(buildMessage);
  return true;
}
bool PortableExecutableFile::AnalyzePE32(PIMAGE_NT_HEADERS32 nh, void *hd) {
  auto oh = reinterpret_cast<IMAGE_OPTIONAL_HEADER32 *>(hd);
  for (auto &sub : subsystemTable) {
    if (sub.index == oh->Subsystem) {
      subsystem = sub.str;
    }
  }
  wsprintfW(linkVersion, L"%d.%d", oh->MajorLinkerVersion,
            oh->MinorLinkerVersion);
  wsprintfW(osVersion, L"%d.%d", oh->MajorOperatingSystemVersion,
            oh->MinorOperatingSystemVersion);
  wsprintfW(subsystemVersion, L"%d.%d", oh->MajorSubsystemVersion,
            oh->MinorSubsystemVersion);
  wsprintfW(imageVersion, L"%d.%d", oh->MajorImageVersion,
            oh->MinorImageVersion);
  auto entry = &(oh->DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER]);
  if (entry->Size != sizeof(IMAGE_COR20_HEADER)) {
    return true;
  }
  auto baseAddr = mv.BaseAddress();
  auto va =
      ImageRvaToVa((PIMAGE_NT_HEADERS)nh, baseAddr, entry->VirtualAddress, 0);
  if ((char *)va - baseAddr > mv.FileSize()) {
    return false;
  }
  auto *clr = reinterpret_cast<IMAGE_COR20_HEADER *>(va);
  auto pm = reinterpret_cast<char *>(ImageRvaToVa(
      (PIMAGE_NT_HEADERS)nh, baseAddr, clr->MetaData.VirtualAddress, 0));
  if ((char *)pm - baseAddr > mv.FileSize()) {
    return false;
  }
  auto buildMessage = pm + 16;
  clrMessage = to_utf16(buildMessage);
  return true;
}

HMODULE KrModule() {
  static HMODULE hModule = GetModuleHandleW(L"kernel32.dll");
  if (hModule == nullptr) {
    OutputDebugStringW(L"GetModuleHandleA failed");
  }
  return hModule;
}

#ifndef _M_X64
class FsRedirection {
public:
  typedef BOOL WINAPI fntype_Wow64DisableWow64FsRedirection(PVOID *OldValue);
  typedef BOOL WINAPI fntype_Wow64RevertWow64FsRedirection(PVOID *OldValue);
  FsRedirection() {
    auto hModule = KrModule();
    auto pfnWow64DisableWow64FsRedirection =
        (fntype_Wow64DisableWow64FsRedirection *)GetProcAddress(
            hModule, "Wow64DisableWow64FsRedirection");
    if (pfnWow64DisableWow64FsRedirection) {
      pfnWow64DisableWow64FsRedirection(&OldValue);
    }
  }
  ~FsRedirection() {
    auto hModule = KrModule();
    auto pfnWow64RevertWow64FsRedirection =
        (fntype_Wow64RevertWow64FsRedirection *)GetProcAddress(
            hModule, "Wow64RevertWow64FsRedirection");
    if (pfnWow64RevertWow64FsRedirection) {
      pfnWow64RevertWow64FsRedirection(&OldValue);
    }
  }

private:
  PVOID OldValue = NULL;
};
#endif

bool PortableExecutableFile::Analyzer() {
#ifndef _M_X64
  FsRedirection fsr;
#endif
  constexpr const size_t minsize =
      sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS);
  if (!mv.Fileview(mPath_)) {
    return false;
  }
  if (mv.FileSize() < minsize) {
    return false;
  }
  auto dh = reinterpret_cast<IMAGE_DOS_HEADER *>(mv.BaseAddress());
  if (minsize + dh->e_lfanew >= mv.FileSize()) {
    return false;
  }
  auto nh =
      reinterpret_cast<IMAGE_NT_HEADERS *>(mv.BaseAddress() + dh->e_lfanew);
  union SigMask {
    DWORD dw;
    char c[4];
  };
  SigMask sigmark;
  sigmark.dw = nh->Signature;
  for (auto &c : sigmark.c) {
    magicStr.push_back(c);
  }
  for (auto &m : machineTable) {
    if (m.index == nh->FileHeader.Machine) {
      machine = m.str;
      break;
    }
  }

  auto var = nh->FileHeader.Characteristics;
  if ((nh->FileHeader.Characteristics & IMAGE_FILE_DLL) == IMAGE_FILE_DLL) {
    mCharacteristics = L"Dynamic Link Library";
  } else if ((nh->FileHeader.Characteristics & IMAGE_FILE_SYSTEM) ==
             IMAGE_FILE_SYSTEM) {
    mCharacteristics = L"System File";
  } else if ((nh->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) ==
             IMAGE_FILE_EXECUTABLE_IMAGE) {
    mCharacteristics = L"Executable File";
  } else {
    mCharacteristics = std::wstring(L"Characteristics value: ") +
                       std::to_wstring(nh->FileHeader.Characteristics);
  }
  if (nh->FileHeader.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER64)) {
    return AnalyzePE64((PIMAGE_NT_HEADERS64)nh, &(nh->OptionalHeader));
  }
  return AnalyzePE32((PIMAGE_NT_HEADERS32)nh, &(nh->OptionalHeader));
}