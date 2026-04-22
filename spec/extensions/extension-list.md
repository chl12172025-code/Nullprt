# Nullprt Extension List

This file defines canonical short and long extension aliases for Nullprt artifacts.

## Source and interfaces
- `.nprt` <-> `.nullprt`
- `.nprti` <-> `.nullprtinterface`
- `.nprtm` <-> `.nullprtmodule`

## Build and binary artifacts
- `.nprtl` <-> `.nullprtlibrary`
- `.nprta` <-> `.nullprtarchive`
- `.nprtso` <-> `.nullprtsharedobject`
- `.nprtdll` <-> `.nullprtdynamiclibrary`
- `.nprtb` <-> `.nullprtbytecode`
- `.nprto` <-> `.nullprtobject`
- `.nprtd` <-> `.nullprtdebug`
- `.nprtmap` <-> `.nullprtmap`

## Diagnostics and profiling
- `.nprtlog` <-> `.nullprtlog`
- `.nprtstats` <-> `.nullprtstatistics`
- `.nprtprof` <-> `.nullprtprofile`
- `.nprtcov` <-> `.nullprtcoverage`

## Tests and docs
- `.nprttest` <-> `.nullprttest`
- `.nprtbench` <-> `.nullprtbenchmark`
- `.nprtdoc` <-> `.nullprtdocument`
- `.nprttut` <-> `.nullprttutorial`
- `.nprtex` <-> `.nullprtexample`

## Config and package
- `.nprtcfg` <-> `.nullprtconfig`
- `.nprtconfig` <-> `.nullprtconfig`
- `.nprtenv` <-> `.nullprtenvironment`
- `.nprtsecret` <-> `.nullprtsecret`
- `.nprtpkg` <-> `.nullprtpackage`
- `.nprtlock` <-> `.nullprtlock`
- `.nprtmanifest` <-> `.nullprtmanifest`

## Security and metadata
- `.nprtkey` <-> `.nullprtkey`
- `.nprtcert` <-> `.nullprtcertificate`
- `.nprtlicense` <-> `.nullprtlicense`
- `.nprtsig` <-> `.nullprtsignature`
- `.nprtmeta` <-> `.nullprtmetadata`

## Interop and IR
- `.nprtc` <-> `.nullprtc`
- `.nprtcpp` <-> `.nullprtcpp`
- `.nprtasm` <-> `.nullprtassembly`
- `.nprtll` <-> `.nullprtllvm`
- `.nprtir` <-> `.nullprtintermediaterepresentation`
- `.nprthir` <-> `.nullprthir`
- `.nprtmir` <-> `.nullprtmir`
- `.nprtlir` <-> `.nullprtlir`

## Notes
- Short form is preferred for source repositories.
- Long form is accepted by parser and tooling as an alias.
- Unknown extensions are treated as opaque assets unless bound by config rules.
