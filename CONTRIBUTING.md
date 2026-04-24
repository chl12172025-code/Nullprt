# Contributing to Nullprt

## CLA Signing Flow
1. Open your first PR.
2. Sign the Contributor License Agreement via the CLA link in PR checks.
3. Wait for `cla/signed` check to pass before review.

## Commit Message Convention
- Format: `type(scope): summary`
- Types: `feat`, `fix`, `docs`, `test`, `refactor`, `build`, `ci`, `chore`.
- Example: `feat(fmt): add deterministic chain wrapping`.

## Code Review Process
1. Author opens PR with template completed.
2. CI must be green, including quality/security gates.
3. At least 1 maintainer approval required.
4. Security-sensitive changes require 2 approvals.

## Dispute Resolution
1. Raise technical concern in PR thread.
2. Escalate to maintainers in `#maintainers-review`.
3. If unresolved in 72h, trigger governance vote recorded in roadmap issue.

## Release Contribution Rules
- No feature merges during release freeze window.
- Only blocker or regression fixes are accepted in RC period.
