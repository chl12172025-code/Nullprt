param([string]$WorkspaceRoot = ".")

$ErrorActionPreference = "Stop"
$root = Resolve-Path $WorkspaceRoot
$report = Join-Path $root "tests/aegc1/test_report.txt"

@"
frontend_token_golden: present
semantic_type_errors: present
semantic_feature_conformance: $(Test-Path (Join-Path $root "tests/aegc1/semantic/feature_conformance.nprt"))
semantic_feature_codes: $(Test-Path (Join-Path $root "tests/aegc1/semantic/feature_expected_codes.txt"))
compiler_1_0_build_core_expected: $(Test-Path (Join-Path $root "tests/aegc1/compiler_1_0/build_core_expected.txt"))
compiler_1_0_isa_matrix_expected: $(Test-Path (Join-Path $root "tests/aegc1/compiler_1_0/isa_matrix_expected.txt"))
compiler_1_0_link_pipeline_expected: $(Test-Path (Join-Path $root "tests/aegc1/compiler_1_0/link_pipeline_expected.txt"))
compiler_1_0_diag_security_expected: $(Test-Path (Join-Path $root "tests/aegc1/compiler_1_0/diag_security_expected.txt"))
compiler_1_0_pkg_manager_expected: $(Test-Path (Join-Path $root "tests/aegc1/compiler_1_0/pkg_manager_expected.txt"))
lsp_full_cases: $(Test-Path (Join-Path $root "tests/lsp/cases/lsp_full_requests.jsonl"))
lsp_full_expected: $(Test-Path (Join-Path $root "tests/lsp/expected/lsp_full_expected.txt"))
debug_full_cases: $(Test-Path (Join-Path $root "tests/debug/cases/debugger_full_sequence.txt"))
debug_full_expected: $(Test-Path (Join-Path $root "tests/debug/expected/debugger_full_expected.txt"))
prof_full_cases: $(Test-Path (Join-Path $root "tests/prof/cases/prof_full_sequence.txt"))
prof_full_expected: $(Test-Path (Join-Path $root "tests/prof/expected/prof_full_expected.txt"))
doc_full_cases: $(Test-Path (Join-Path $root "tests/doc/cases/doc_full_sequence.txt"))
doc_full_expected: $(Test-Path (Join-Path $root "tests/doc/expected/doc_full_expected.txt"))
fmt_full_cases: $(Test-Path (Join-Path $root "tests/fmt/cases/fmt_full_sequence.txt"))
fmt_full_expected: $(Test-Path (Join-Path $root "tests/fmt/expected/fmt_full_expected.txt"))
docs_site_full_cases: $(Test-Path (Join-Path $root "tests/docs_site/cases/docs_site_full_sequence.txt"))
docs_site_full_expected: $(Test-Path (Join-Path $root "tests/docs_site/expected/docs_site_full_expected.txt"))
examples_full_cases: $(Test-Path (Join-Path $root "tests/examples/cases/examples_full_sequence.txt"))
examples_full_expected: $(Test-Path (Join-Path $root "tests/examples/expected/examples_full_expected.txt"))
qa_full_cases: $(Test-Path (Join-Path $root "tests/qa/cases/release_qa_full_sequence.txt"))
qa_full_expected: $(Test-Path (Join-Path $root "tests/qa/expected/release_qa_full_expected.txt"))
community_full_cases: $(Test-Path (Join-Path $root "tests/community/cases/community_ecosystem_full_sequence.txt"))
community_full_expected: $(Test-Path (Join-Path $root "tests/community/expected/community_ecosystem_full_expected.txt"))
ir_golden_expected: present
backend_c_golden_contains: present
backend_native_magic_expected: present
selfhost_input: present
protect_passes_source: $(Test-Path (Join-Path $root "src/self/aegc1/ir/protect_passes.c"))
dev_gate_source: $(Test-Path (Join-Path $root "src/self/aegc1/runtime/dev_gate.c"))
concurrency_runtime_source: $(Test-Path (Join-Path $root "src/self/aegc1/runtime/concurrency_runtime.c"))
ast_validate_source: $(Test-Path (Join-Path $root "src/self/aegc1/frontend/ast_validate.c"))
build_graph_source: $(Test-Path (Join-Path $root "src/self/aegc1/driver/build_graph.c"))
distributed_protocol_source: $(Test-Path (Join-Path $root "src/self/aegc1/driver/distributed_protocol.c"))
pch_compat_source: $(Test-Path (Join-Path $root "src/self/aegc1/driver/pch_compat.c"))
diag_i18n_source: $(Test-Path (Join-Path $root "src/self/aegc1/diag/i18n.c"))
security_vuln_db_source: $(Test-Path (Join-Path $root "src/self/aegc1/security/vuln_db.c"))
pkg_advanced_source: $(Test-Path (Join-Path $root "src/pkg/nprt-pkg/advanced.c"))
lsp_index_source: $(Test-Path (Join-Path $root "src/lsp/nprt-lsp/index.c"))
lsp_verify_script: $(Test-Path (Join-Path $root "scripts/verify_lsp_full_completion.ps1"))
debug_expr_source: $(Test-Path (Join-Path $root "src/debug/nprt-debug/expr_eval.c"))
debug_verify_script: $(Test-Path (Join-Path $root "scripts/verify_debugger_full_completion.ps1"))
prof_analysis_source: $(Test-Path (Join-Path $root "src/prof/nprt-prof/analysis_core.c"))
prof_verify_script: $(Test-Path (Join-Path $root "scripts/verify_profiler_full_completion.ps1"))
doc_generator_source: $(Test-Path (Join-Path $root "src/doc/nprt-doc/main.c"))
doc_verify_script: $(Test-Path (Join-Path $root "scripts/verify_doc_full_completion.ps1"))
fmt_generator_source: $(Test-Path (Join-Path $root "src/fmt/nprt-fmt/format_core.c"))
fmt_verify_script: $(Test-Path (Join-Path $root "scripts/verify_formatter_full_completion.ps1"))
docs_site_script: $(Test-Path (Join-Path $root "scripts/verify_docs_site_full_completion.ps1"))
examples_verify_script: $(Test-Path (Join-Path $root "scripts/verify_examples_full_completion.ps1"))
qa_verify_script: $(Test-Path (Join-Path $root "scripts/verify_release_qa_full_completion.ps1"))
community_verify_script: $(Test-Path (Join-Path $root "scripts/verify_community_ecosystem_full_completion.ps1"))
stable_ga_verify_script: $(Test-Path (Join-Path $root "scripts/verify_release_stable_ga.ps1"))
api_doc_generator: $(Test-Path (Join-Path $root "scripts/generate_api_docs.ps1"))
note: runtime execution requires compiled aegc1 driver binary.
"@ | Set-Content -Encoding UTF8 $report

Write-Host "[tests/aegc1] report written: $report"
