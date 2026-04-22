#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum NprtCfgType {
  NPRT_CFG_STRING = 0,
  NPRT_CFG_INT,
  NPRT_CFG_FLOAT,
  NPRT_CFG_BOOL
} NprtCfgType;

typedef struct NprtCfgEntry {
  char* key;
  char* value;
  NprtCfgType type;
} NprtCfgEntry;

typedef struct NprtConfig {
  NprtCfgEntry* entries;
  size_t len;
} NprtConfig;

static char* cfg_strdup(const char* s) {
  size_t n = strlen(s);
  char* p = (char*)malloc(n + 1);
  if (!p) return NULL;
  memcpy(p, s, n + 1);
  return p;
}

static void trim(char* s) {
  char* p = s;
  while (*p && isspace((unsigned char)*p)) p++;
  if (p != s) memmove(s, p, strlen(p) + 1);
  size_t n = strlen(s);
  while (n > 0 && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
}

static NprtCfgType infer_type(const char* v) {
  if (!strcmp(v, "true") || !strcmp(v, "false")) return NPRT_CFG_BOOL;
  bool seen_dot = false;
  if (!*v) return NPRT_CFG_STRING;
  for (const char* p = v; *p; p++) {
    if (*p == '.') { seen_dot = true; continue; }
    if (!isdigit((unsigned char)*p) && *p != '-') return NPRT_CFG_STRING;
  }
  return seen_dot ? NPRT_CFG_FLOAT : NPRT_CFG_INT;
}

static bool cfg_set(NprtConfig* cfg, const char* key, const char* value) {
  for (size_t i = 0; i < cfg->len; i++) {
    if (!strcmp(cfg->entries[i].key, key)) {
      free(cfg->entries[i].value);
      cfg->entries[i].value = cfg_strdup(value);
      cfg->entries[i].type = infer_type(value);
      return cfg->entries[i].value != NULL;
    }
  }
  NprtCfgEntry* n = (NprtCfgEntry*)realloc(cfg->entries, sizeof(NprtCfgEntry) * (cfg->len + 1));
  if (!n) return false;
  cfg->entries = n;
  cfg->entries[cfg->len].key = cfg_strdup(key);
  cfg->entries[cfg->len].value = cfg_strdup(value);
  cfg->entries[cfg->len].type = infer_type(value);
  if (!cfg->entries[cfg->len].key || !cfg->entries[cfg->len].value) return false;
  cfg->len++;
  return true;
}

bool nprt_cfg_parse_string(const char* text, NprtConfig* out) {
  memset(out, 0, sizeof(*out));
  char* buf = cfg_strdup(text ? text : "");
  if (!buf) return false;
  char section[128] = {0};
  char* save = NULL;
  for (char* line = strtok_s(buf, "\n", &save); line; line = strtok_s(NULL, "\n", &save)) {
    trim(line);
    if (!line[0] || (line[0] == '/' && line[1] == '/')) continue;
    if (line[0] == '[') {
      char* end = strchr(line, ']');
      if (!end) continue;
      *end = '\0';
      strncpy(section, line + 1, sizeof(section) - 1);
      continue;
    }
    char* eq = strchr(line, '=');
    if (!eq) continue;
    *eq = '\0';
    char key[256];
    char val[1024];
    strncpy(key, line, sizeof(key) - 1);
    strncpy(val, eq + 1, sizeof(val) - 1);
    key[sizeof(key) - 1] = '\0';
    val[sizeof(val) - 1] = '\0';
    trim(key);
    trim(val);
    char full_key[512];
    if (section[0]) snprintf(full_key, sizeof(full_key), "%s.%s", section, key);
    else snprintf(full_key, sizeof(full_key), "%s", key);
    if (!cfg_set(out, full_key, val)) {
      free(buf);
      return false;
    }
  }
  free(buf);
  return true;
}

bool nprt_cfg_load_file(const char* path, NprtConfig* out) {
  FILE* f = fopen(path, "rb");
  if (!f) return false;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (n < 0) { fclose(f); return false; }
  char* buf = (char*)malloc((size_t)n + 1);
  if (!buf) { fclose(f); return false; }
  size_t got = fread(buf, 1, (size_t)n, f);
  fclose(f);
  buf[got] = '\0';
  bool ok = nprt_cfg_parse_string(buf, out);
  free(buf);
  return ok;
}

bool nprt_cfg_merge(NprtConfig* base, const NprtConfig* overlay) {
  for (size_t i = 0; i < overlay->len; i++) {
    if (!cfg_set(base, overlay->entries[i].key, overlay->entries[i].value)) return false;
  }
  return true;
}

bool nprt_cfg_apply_env_overrides(NprtConfig* cfg) {
  char tmp[512];
  for (size_t i = 0; i < cfg->len; i++) {
    snprintf(tmp, sizeof(tmp), "NPRT_CFG_%s", cfg->entries[i].key);
    for (char* p = tmp; *p; p++) {
      if (*p == '.') *p = '_';
      *p = (char)toupper((unsigned char)*p);
    }
    const char* v = getenv(tmp);
    if (v && v[0]) {
      if (!cfg_set(cfg, cfg->entries[i].key, v)) return false;
    }
  }
  return true;
}

bool nprt_cfg_apply_cli_override(NprtConfig* cfg, const char* expr) {
  char buf[1024];
  strncpy(buf, expr, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  char* eq = strchr(buf, '=');
  if (!eq) return false;
  *eq = '\0';
  char* key = buf;
  char* val = eq + 1;
  trim(key);
  trim(val);
  return cfg_set(cfg, key, val);
}

const char* nprt_cfg_get_string(const NprtConfig* cfg, const char* key, const char* def) {
  for (size_t i = 0; i < cfg->len; i++) if (!strcmp(cfg->entries[i].key, key)) return cfg->entries[i].value;
  return def;
}

long long nprt_cfg_get_int(const NprtConfig* cfg, const char* key, long long def) {
  const char* v = nprt_cfg_get_string(cfg, key, NULL);
  return v ? strtoll(v, NULL, 10) : def;
}

double nprt_cfg_get_float(const NprtConfig* cfg, const char* key, double def) {
  const char* v = nprt_cfg_get_string(cfg, key, NULL);
  return v ? strtod(v, NULL) : def;
}

bool nprt_cfg_get_bool(const NprtConfig* cfg, const char* key, bool def) {
  const char* v = nprt_cfg_get_string(cfg, key, NULL);
  if (!v) return def;
  return !strcmp(v, "true") || !strcmp(v, "1");
}

bool nprt_cfg_require(const NprtConfig* cfg, const char* key) {
  return nprt_cfg_get_string(cfg, key, NULL) != NULL;
}

bool nprt_cfg_validate(const NprtConfig* cfg, char* err, size_t err_cap) {
  if (!nprt_cfg_require(cfg, "package.name")) {
    snprintf(err, err_cap, "missing required key: package.name");
    return false;
  }
  if (!nprt_cfg_require(cfg, "package.version")) {
    snprintf(err, err_cap, "missing required key: package.version");
    return false;
  }
  return true;
}

void nprt_cfg_free(NprtConfig* cfg) {
  for (size_t i = 0; i < cfg->len; i++) {
    free(cfg->entries[i].key);
    free(cfg->entries[i].value);
  }
  free(cfg->entries);
  cfg->entries = NULL;
  cfg->len = 0;
}
