#include <stdbool.h>
#include <string.h>

bool a1_license_is_compatible(const char* root_license, const char* dep_license) {
  if (!root_license || !dep_license) return false;
  if (strcmp(root_license, "MIT") == 0) return true;
  if (strcmp(root_license, "Apache-2.0") == 0 && strcmp(dep_license, "GPL-3.0") == 0) return false;
  if (strcmp(root_license, "Proprietary") == 0 && strstr(dep_license, "GPL")) return false;
  return true;
}

const char* a1_license_policy_reason(const char* root_license, const char* dep_license) {
  if (a1_license_is_compatible(root_license, dep_license)) return "compatible";
  return "copyleft or policy conflict";
}
