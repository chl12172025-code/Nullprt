#include "extension_registry.h"

#include <string.h>

static const NprtExtensionInfo kNprtExtensions[] = {
    {".nprt", ".nullprt", "source"},
    {".nprti", ".nullprtinterface", "source"},
    {".nprtm", ".nullprtmodule", "source"},
    {".nprtl", ".nullprtlibrary", "binary"},
    {".nprta", ".nullprtarchive", "binary"},
    {".nprtso", ".nullprtsharedobject", "binary"},
    {".nprtdll", ".nullprtdynamiclibrary", "binary"},
    {".nprtb", ".nullprtbytecode", "binary"},
    {".nprto", ".nullprtobject", "binary"},
    {".nprtd", ".nullprtdebug", "binary"},
    {".nprtmap", ".nullprtmap", "binary"},
    {".nprtlog", ".nullprtlog", "diagnostics"},
    {".nprtstats", ".nullprtstatistics", "diagnostics"},
    {".nprtprof", ".nullprtprofile", "diagnostics"},
    {".nprtcov", ".nullprtcoverage", "diagnostics"},
    {".nprttest", ".nullprttest", "qa"},
    {".nprtbench", ".nullprtbenchmark", "qa"},
    {".nprtdoc", ".nullprtdocument", "docs"},
    {".nprttut", ".nullprttutorial", "docs"},
    {".nprtex", ".nullprtexample", "docs"},
    {".nprtcfg", ".nullprtconfig", "config"},
    {".nprtconfig", ".nullprtconfig", "config"},
    {".nprtenv", ".nullprtenvironment", "config"},
    {".nprtsecret", ".nullprtsecret", "config"},
    {".nprtpkg", ".nullprtpackage", "package"},
    {".nprtlock", ".nullprtlock", "package"},
    {".nprtmanifest", ".nullprtmanifest", "package"},
    {".nprtkey", ".nullprtkey", "security"},
    {".nprtcert", ".nullprtcertificate", "security"},
    {".nprtlicense", ".nullprtlicense", "security"},
    {".nprtsig", ".nullprtsignature", "security"},
    {".nprtmeta", ".nullprtmetadata", "metadata"},
    {".nprtc", ".nullprtc", "interop"},
    {".nprtcpp", ".nullprtcpp", "interop"},
    {".nprtasm", ".nullprtassembly", "interop"},
    {".nprtll", ".nullprtllvm", "interop"},
    {".nprtir", ".nullprtintermediaterepresentation", "ir"},
    {".nprthir", ".nullprthir", "ir"},
    {".nprtmir", ".nullprtmir", "ir"},
    {".nprtlir", ".nullprtlir", "ir"},
};

size_t nprt_extension_count(void) {
  return sizeof(kNprtExtensions) / sizeof(kNprtExtensions[0]);
}

const NprtExtensionInfo* nprt_extension_lookup(const char* ext) {
  size_t i = 0;
  if (!ext || !ext[0]) return NULL;
  for (i = 0; i < nprt_extension_count(); i++) {
    const NprtExtensionInfo* item = &kNprtExtensions[i];
    if (!strcmp(ext, item->short_ext) || !strcmp(ext, item->long_ext)) {
      return item;
    }
  }
  return NULL;
}

bool nprt_extension_is_known(const char* ext) {
  return nprt_extension_lookup(ext) != NULL;
}

const char* nprt_extension_normalize(const char* ext) {
  const NprtExtensionInfo* info = nprt_extension_lookup(ext);
  return info ? info->short_ext : ext;
}
