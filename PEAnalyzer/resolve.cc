///
#include "base.hpp"
#include "reparserpoint.hpp"
#include "resolve.hpp"
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
                        IID_IShellLink, (void **)&link)) != S_OK) {
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
  //
  return std::nullopt;
}
} // namespace resolve