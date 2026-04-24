param([string]$WorkspaceRoot = ".")

$ErrorActionPreference = "Stop"
$root = Resolve-Path $WorkspaceRoot
$report = Join-Path $root "scripts/verify_community_ecosystem_full_completion_report.txt"
$failed = $false

function Invoke-Check {
  param(
    [string]$Name,
    [bool]$Condition,
    [string]$Detail
  )
  $status = if ($Condition) { "PASS" } else { "FAIL" }
  Add-Content -Path $report -Value ("{0}: {1} ({2})" -f $Name, $status, $Detail)
  if (-not $Condition) { $script:failed = $true }
}

function Must-Contain {
  param(
    [string]$Path,
    [string[]]$Keywords
  )
  if (-not (Test-Path $Path)) { return $false }
  $text = Get-Content -Raw -Path $Path
  foreach ($k in $Keywords) {
    if ($text -notmatch [regex]::Escape($k)) { return $false }
  }
  return $true
}

Set-Content -Path $report -Encoding UTF8 -Value "verify_community_ecosystem_full_completion"

$contributing = Join-Path $root "CONTRIBUTING.md"
$conduct = Join-Path $root "CODE_OF_CONDUCT.md"
$roadmap = Join-Path $root "ROADMAP.md"
$blog = Join-Path $root "docs/community/technical-blog-program.md"
$paper = Join-Path $root "docs/community/academic-paper-plan.md"
$iconReadme = Join-Path $root "docs/icons/README.md"
$nightlyWorkflow = Join-Path $root ".github/workflows/nullprt-nightly.yml"
$releaseWorkflow = Join-Path $root ".github/workflows/release-alpha.yml"
$issueDir = Join-Path $root ".github/ISSUE_TEMPLATE"
$prTemplate = Join-Path $root ".github/pull_request_template.md"

Invoke-Check "community_docs_present" (
  (Test-Path $contributing) -and
  (Test-Path $conduct) -and
  (Test-Path $roadmap) -and
  (Test-Path $blog) -and
  (Test-Path $paper)
) "core community documents"

Invoke-Check "contributing_governance_sections" (Must-Contain $contributing @(
  "CLA Signing Flow",
  "Commit Message Convention",
  "Code Review Process",
  "Dispute Resolution"
)) "cla + commit + review + dispute"

Invoke-Check "conduct_reporting_sections" (Must-Contain $conduct @(
  "Reporting Channels",
  "Handling Process",
  "Enforcement Actions",
  "Confidentiality Commitment"
)) "report + process + enforcement + confidentiality"

Invoke-Check "roadmap_planning_sections" (Must-Contain $roadmap @(
  "Milestones and Timeline",
  "Feature Priority Voting",
  "Release Cadence",
  "Community Intake"
)) "timeline + vote + cadence + intake"

Invoke-Check "issue_templates_present" (
  (Test-Path (Join-Path $issueDir "bug_report.yml")) -and
  (Test-Path (Join-Path $issueDir "feature_request.yml")) -and
  (Test-Path (Join-Path $issueDir "security_report.yml")) -and
  (Test-Path (Join-Path $issueDir "performance_issue.yml"))
) "bug/feature/security/perf templates"

Invoke-Check "pr_template_present" (Must-Contain $prTemplate @(
  "Change Description",
  "Test Coverage",
  "Documentation Updates",
  "Breaking Changes"
)) "pr quality sections"

Invoke-Check "blog_program_present" (Must-Contain $blog @(
  "Publishing Platform Accounts",
  "Content Calendar",
  "SEO Optimization",
  "Comment and Interaction Management"
)) "blog account/calendar/seo/comments"

Invoke-Check "paper_plan_present" (Must-Contain $paper @(
  "Target Venues",
  "Paper Structure",
  "Experimental Data Preparation",
  "Peer Review Invitations"
)) "venue/structure/data/review"

Invoke-Check "icon_marketplace_publish_doc" (Must-Contain $iconReadme @(
  "Publish to VS Code Marketplace",
  "vsce publish"
)) "icon theme marketplace publishing"

Invoke-Check "win_arm64_native_runner_plan" (Must-Contain $releaseWorkflow @(
  "NPRT_ENABLE_WIN_ARM64_NATIVE",
  "self-hosted",
  "ARM64"
)) "native runner gated job in release workflow"

Invoke-Check "nightly_prerelease_upload_configured" (Must-Contain $nightlyWorkflow @(
  "publish-nightly-prerelease",
  "softprops/action-gh-release",
  "prerelease: true"
)) "nightly upload to prerelease channel"

if ($failed) {
  Write-Host "[verify-community-full] FAILED"
  exit 1
}

Write-Host "[verify-community-full] PASS"
exit 0
