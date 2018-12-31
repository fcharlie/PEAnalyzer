////////////////
#ifndef FILEVIEW_SHELLLINK_H
#define FILEVIEW_SHELLLINK_H
#include <cstdint>

namespace shl {
// Shell link flags
// Thanks:
// https://github.com/reactos/reactos/blob/bfcbda227f99c1b59e8ed71f5e0f59f793d496a1/sdk/include/reactos/undocshell.h#L800
enum : uint32_t {
  SldfNone = 0x00000000,
  HasLinkTargetIDList = 0x00000001,
  HasLinkInfo = 0x00000002,
  HasName = 0x00000004,
  HasRelativePath = 0x00000008,
  HasWorkingDir = 0x00000010,
  HasArguments = 0x00000020,
  HasIconLocation = 0x00000040,
  IsUnicode = 0x00000080,
  ForceNoLinkInfo = 0x00000100,
  HasExpString = 0x00000200,
  RunInSeparateProcess = 0x00000400,
  Unused1 = 0x00000800,
  HasDrawinID = 0x00001000,
  RunAsUser = 0x00002000,
  HasExpIcon = 0x00004000,
  NoPidlAlias = 0x00008000,
  Unused2 = 0x00010000,
  RunWithShimLayer = 0x00020000,
  ForceNoLinkTrack = 0x00040000,
  EnableTargetMetadata = 0x00080000,
  DisableLinkPathTarcking = 0x00100000,
  DisableKnownFolderTarcking = 0x00200000,
  DisableKnownFolderAlia = 0x00400000,
  AllowLinkToLink = 0x00800000,
  UnaliasOnSave = 0x01000000,
  PreferEnvironmentPath = 0x02000000,
  KeepLocalIDListForUNCTarget = 0x04000000,
  PersistVolumeIDRelative = 0x08000000,
  SldfInvalid = 0x0ffff7ff,
  Reserved = 0x80000000
};

#pragma pack(1)
/*
SHELL_LINK = SHELL_LINK_HEADER [LINKTARGET_IDLIST] [LINKINFO]
 [STRING_DATA] *EXTRA_DATA
*/

struct filetime_t {
  uint32_t low;
  uint32_t high;
};

struct shell_link_t {
  uint32_t dwSize;
  uint8_t uuid[16];
  uint32_t linkflags;
  uint32_t fileattr;
  filetime_t createtime;
  filetime_t accesstime;
  filetime_t writetime;
  uint32_t filesize;
  uint32_t iconindex;
  uint32_t showcommand;
  uint16_t hotkey;
  uint16_t reserved1;
  uint32_t reserved2;
  uint32_t reserved3;
};

enum link_info_flags {
  VolumeIDAndLocalBasePath = 0x00000001,
  CommonNetworkRelativeLinkAndPathSuffix = 0x00000002
};

struct link_info_t {
  uint32_t cbSize;
  uint32_t cbHeaderSize;
  uint32_t dwFlags;
  uint32_t cbVolumeIDOffset;
  uint32_t cbLocalBasePathOffset;
  uint32_t cbCommonNetworkRelativeLinkOffset;
  uint32_t cbCommonPathSuffixOffset;
  uint32_t cbLocalBasePathOffsetUnicode;
  uint32_t cbCommonPathSuffixOffsetUnicode;
};

/*
 IDList ItemIDSize (2 bytes):
Data (variable):
 */
#pragma pack()

} // namespace shl

#endif
