#include <stdbool.h>
#include <string.h>

const char* a1_diag_translate(const char* locale, const char* code) {
  if (!code) return "unknown diagnostic";
  if (!locale || strcmp(locale, "en-US") == 0) return code;
  if (strcmp(locale, "zh-CN") == 0) {
    if (strcmp(code, "E1001") == 0) return "依赖类型需要泛型约束。";
    if (strcmp(code, "E1002") == 0) return "细化类型需要契约检查。";
  }
  if (strcmp(locale, "ja-JP") == 0) {
    if (strcmp(code, "E1001") == 0) return "依存型にはジェネリック制約が必要です。";
  }
  return code;
}

bool a1_diag_locale_supported(const char* locale) {
  if (!locale) return false;
  return strcmp(locale, "en-US") == 0 ||
         strcmp(locale, "zh-CN") == 0 ||
         strcmp(locale, "ja-JP") == 0;
}
