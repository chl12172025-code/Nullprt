#pragma once

#include <stdbool.h>
#include <stddef.h>

bool npkg_download_chunk_resume(const char* url, const char* out_path, size_t chunk_size, size_t* resumed_from);
bool npkg_parallel_download_throttle(size_t max_parallel, size_t bytes_per_sec, char* out_msg, size_t out_msg_cap);
bool npkg_package_strip_symbols(const char* in_pkg, const char* out_pkg);
bool npkg_upload_chunk_resume(const char* in_path, const char* endpoint, size_t chunk_size, size_t* resumed_from);
bool npkg_metadata_sign(const char* metadata_path, const char* key_id, char* out_sig_hex, size_t out_sig_cap);
bool npkg_push_deprecation_notice(const char* package, const char* version, const char* message, const char* out_path);
bool npkg_install_transaction_with_rollback(const char* script_path, const char* rollback_path);
bool npkg_install_symlink_with_permissions(const char* from, const char* to, char* out_note, size_t out_note_cap);
bool npkg_merge_lockfiles(const char* left_path, const char* right_path, const char* out_path);
