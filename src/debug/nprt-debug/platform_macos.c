#if defined(__APPLE__)
int nprt_dbg_platform_attach(unsigned long pid) {
  (void)pid;
  // Mach-based backend placeholder.
  return 1;
}
#endif
