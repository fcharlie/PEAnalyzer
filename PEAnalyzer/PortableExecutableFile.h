

#ifndef PORTABLE_EXECUTABLE_FILE_H
#define PORTABLE_EXECUTABLE_FILE_H
#include <Windows.h>
#include <string>

/*
#define IO_REPARSE_TAG_MOUNT_POINT              (0xA0000003L)
#define IO_REPARSE_TAG_HSM                      (0xC0000004L)
#define IO_REPARSE_TAG_HSM2                     (0x80000006L)
#define IO_REPARSE_TAG_SIS                      (0x80000007L)
#define IO_REPARSE_TAG_WIM                      (0x80000008L)
#define IO_REPARSE_TAG_CSV                      (0x80000009L)
#define IO_REPARSE_TAG_DFS                      (0x8000000AL)
#define IO_REPARSE_TAG_SYMLINK                  (0xA000000CL)
#define IO_REPARSE_TAG_DFSR                     (0x80000012L)
#define IO_REPARSE_TAG_DEDUP                    (0x80000013L)
#define IO_REPARSE_TAG_NFS                      (0x80000014L)
#define IO_REPARSE_TAG_FILE_PLACEHOLDER         (0x80000015L)
#define IO_REPARSE_TAG_WOF                      (0x80000017L)
#define IO_REPARSE_TAG_WCI                      (0x80000018L)
#define IO_REPARSE_TAG_WCI_1                    (0x90001018L)
#define IO_REPARSE_TAG_GLOBAL_REPARSE           (0xA0000019L)
#define IO_REPARSE_TAG_CLOUD                    (0x9000001AL)
#define IO_REPARSE_TAG_CLOUD_1                  (0x9000101AL)
#define IO_REPARSE_TAG_CLOUD_2                  (0x9000201AL)
#define IO_REPARSE_TAG_CLOUD_3                  (0x9000301AL)
#define IO_REPARSE_TAG_CLOUD_4                  (0x9000401AL)
#define IO_REPARSE_TAG_CLOUD_5                  (0x9000501AL)
#define IO_REPARSE_TAG_CLOUD_6                  (0x9000601AL)
#define IO_REPARSE_TAG_CLOUD_7                  (0x9000701AL)
#define IO_REPARSE_TAG_CLOUD_8                  (0x9000801AL)
#define IO_REPARSE_TAG_CLOUD_9                  (0x9000901AL)
#define IO_REPARSE_TAG_CLOUD_A                  (0x9000A01AL)
#define IO_REPARSE_TAG_CLOUD_B                  (0x9000B01AL)
#define IO_REPARSE_TAG_CLOUD_C                  (0x9000C01AL)
#define IO_REPARSE_TAG_CLOUD_D                  (0x9000D01AL)
#define IO_REPARSE_TAG_CLOUD_E                  (0x9000E01AL)
#define IO_REPARSE_TAG_CLOUD_F                  (0x9000F01AL)
#define IO_REPARSE_TAG_CLOUD_MASK               (0x0000F000L)
#define IO_REPARSE_TAG_APPEXECLINK              (0x8000001BL)
#define IO_REPARSE_TAG_PROJFS                   (0x9000001CL)
#define IO_REPARSE_TAG_STORAGE_SYNC             (0x8000001EL)
#define IO_REPARSE_TAG_WCI_TOMBSTONE            (0xA000001FL)
#define IO_REPARSE_TAG_UNHANDLED                (0x80000020L)
#define IO_REPARSE_TAG_ONEDRIVE                 (0x80000021L)
#define IO_REPARSE_TAG_PROJFS_TOMBSTONE         (0xA0000022L)
#define IO_REPARSE_TAG_AF_UNIX                  (0x80000023L)

/// Windows Linux Subsystem
#define IO_REPARSE_TAG_LX_FIFO                  (0x80000024L)
#define IO_REPARSE_TAG_LX_CHR                   (0x80000025L)
#define IO_REPARSE_TAG_LX_BLK                   (0x80000026L)
*/

#ifndef IO_REPARSE_TAG_APPEXECLINK
#define IO_REPARSE_TAG_APPEXECLINK (0x8000001BL)
#endif

// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/ntifs/ns-ntifs-_reparse_data_buffer

typedef struct _REPARSE_DATA_BUFFER {
  ULONG ReparseTag;         // Reparse tag type
  USHORT ReparseDataLength; // Length of the reparse data
  USHORT Reserved;          // Used internally by NTFS to store remaining length

  union {
    // Structure for IO_REPARSE_TAG_SYMLINK
    // Handled by nt!IoCompleteRequest
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      ULONG Flags;
      WCHAR PathBuffer[1];
    } SymbolicLinkReparseBuffer;

    // Structure for IO_REPARSE_TAG_MOUNT_POINT
    // Handled by nt!IoCompleteRequest
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      WCHAR PathBuffer[1];
    } MountPointReparseBuffer;

    // Structure for IO_REPARSE_TAG_WIM
    // Handled by wimmount!FPOpenReparseTarget->wimserv.dll
    // (wimsrv!ImageExtract)
    struct {
      GUID ImageGuid;           // GUID of the mounted VIM image
      BYTE ImagePathHash[0x14]; // Hash of the path to the file within the image
    } WimImageReparseBuffer;

    // Structure for IO_REPARSE_TAG_WOF
    // Handled by FSCTL_GET_EXTERNAL_BACKING, FSCTL_SET_EXTERNAL_BACKING in NTFS
    // (Windows 10+)
    struct {
      //-- WOF_EXTERNAL_INFO --------------------
      ULONG Wof_Version;  // Should be 1 (WOF_CURRENT_VERSION)
      ULONG Wof_Provider; // Should be 2 (WOF_PROVIDER_FILE)

      //-- FILE_PROVIDER_EXTERNAL_INFO_V1 --------------------
      ULONG FileInfo_Version; // Should be 1 (FILE_PROVIDER_CURRENT_VERSION)
      ULONG
      FileInfo_Algorithm; // Usually 0 (FILE_PROVIDER_COMPRESSION_XPRESS4K)
    } WofReparseBuffer;

    // Structure for IO_REPARSE_TAG_APPEXECLINK
    struct {
      ULONG StringCount;   // Number of the strings in the StringList, separated
                           // by '\0'
      WCHAR StringList[1]; // Multistring (strings separated by '\0', terminated
                           // by '\0\0')
    } AppExecLinkReparseBuffer;

    // Dummy structure
    struct {
      UCHAR DataBuffer[1];
    } GenericReparseBuffer;
  } DUMMYUNIONNAME;
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

#ifndef PROCESSOR_ARCHITECTURE_ARM64
#define PROCESSOR_ARCHITECTURE_ARM64 12
#endif
#ifndef PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64
#define PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64 13
#endif
#ifndef PROCESSOR_ARCHITECTURE_IA32_ON_ARM64
#define PROCESSOR_ARCHITECTURE_IA32_ON_ARM64 14
#endif

#ifndef IMAGE_FILE_MACHINE_TARGET_HOST
#define IMAGE_FILE_MACHINE_TARGET_HOST                                         \
  0x0001 // Useful for indicating we want to interact with the host and not a
         // WoW guest.
#endif

/// #define PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64   13
#ifndef IMAGE_FILE_MACHINE_ARM64
//// IMAGE_FILE_MACHINE_ARM64 is Windows
#define IMAGE_FILE_MACHINE_ARM64 0xAA64 // ARM64 Little-Endian
#endif
#ifndef IMAGE_SUBSYSTEM_XBOX_CODE_CATALOG
#define IMAGE_SUBSYSTEM_XBOX_CODE_CATALOG 17 // XBOX Code Catalog
#endif

typedef enum ReplacesGeneralNumericDefines {
// Directory entry macro for CLR data.
#ifndef IMAGE_DIRECTORY_ENTRY_COMHEADER
  IMAGE_DIRECTORY_ENTRY_COMHEADER = 14,
#endif // IMAGE_DIRECTORY_ENTRY_COMHEADER
} ReplacesGeneralNumericDefines;

const wchar_t *ArchitectureName(int id);

class Memview {
public:
  Memview() = default;
  Memview(const Memview &) = delete;
  Memview &operator=(const Memview &) = delete;
  ~Memview() {
    if (ismaped) {
      UnmapViewOfFile(hMap);
    }
    if (hMap != INVALID_HANDLE_VALUE) {
      CloseHandle(hMap);
    }
    if (hFile != INVALID_HANDLE_VALUE) {
      CloseHandle(hFile);
    }
  }
  bool Fileview(const std::wstring_view &path) {
    hFile = CreateFileW(path.data(), GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
      return false;
    }
    LARGE_INTEGER li;
    if (GetFileSizeEx(hFile, &li)) {
      filesize = li.QuadPart;
    }
    hMap = CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (hMap == INVALID_HANDLE_VALUE) {
      return false;
    }
    baseAddress = ::MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (baseAddress == nullptr) {
      return false;
    }
    ismaped = true;
    return true;
  }
  int64_t FileSize() const { return filesize; }
  char *BaseAddress() { return reinterpret_cast<char *>(baseAddress); }

private:
  HANDLE hFile{INVALID_HANDLE_VALUE};
  HANDLE hMap{INVALID_HANDLE_VALUE};
  LPVOID baseAddress{nullptr};
  int64_t filesize{0};
  bool ismaped{false};
};

template <typename CharT>
bool EndsWith(const std::basic_string<CharT> &str, const CharT *cstr,
              size_t len) {
  if (str.size() < len) {
    return false;
  }
  return str.compare(str.size() - len, len, cstr) == 0;
}

bool ReadShellLink(const std::wstring &shlink, std::wstring &target);
bool Readlink(const std::wstring &symfile, std::wstring &realfile);

class PortableExecutableFile {
private:
  std::wstring mPath_;
  std::wstring magicStr;
  std::wstring machine;
  std::wstring subsystem;
  std::wstring mCharacteristics;
  std::wstring clrMessage;
  wchar_t linkVersion[16];
  wchar_t osVersion[16];
  wchar_t subsystemVersion[16];
  wchar_t imageVersion[16];
  Memview mv;
  bool AnalyzePE64(PIMAGE_NT_HEADERS64 nh, void *hd);
  bool AnalyzePE32(PIMAGE_NT_HEADERS32 nh, void *hd);

public:
  PortableExecutableFile(const std::wstring &mPath);
  bool Analyzer();
  const std::wstring &GetSignature() { return magicStr; }
  const std::wstring &GetMachine() { return machine; }
  const std::wstring &GetSubSystem() { return subsystem; }
  const std::wstring &GetCharacteristics() { return mCharacteristics; }
  const std::wstring &GetCLRMessage() { return clrMessage; }
  const wchar_t *GetLinkerVersion() { return linkVersion; }
  const wchar_t *GetOSVersion() { return osVersion; }
  const wchar_t *GetSubSystemVersion() { return subsystemVersion; }
  const wchar_t *GetImageVersion() { return imageVersion; }
};

#endif
