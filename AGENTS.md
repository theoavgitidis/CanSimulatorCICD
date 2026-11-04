# Repository guidance

## Project context

- This C++20/CMake repository contains `CanSim`, a vehicle CAN telemetry simulator, and `CanDisplay`, a portable receiver and display model with a host terminal preview.
- Read `HANDOFF.md` for resume notes, `TODO.md` for the backlog, and the relevant subproject README before changing behavior. Hardware wiring and bench setup are documented in `docs/hardware/`.
- Keep the portable encoder, scheduler, decoder, receiver, and display model independent of ESP32 and graphics drivers. Put hardware-specific code behind small adapters.
- Treat `CanSim/config/can-signals.yaml` as the CAN contract. Update simulator-to-display contract coverage when changing IDs, scaling, byte layout, or message periods.
- Reuse scenario and simulator logic in the CLI and host preview. Use supplied elapsed time for testable scenario progression.

## Build and validation

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

- Use the existing `.clang-format` style. The `format` build target formats C++ sources; CTest includes formatting checks when `clang-format` is installed.
- Run relevant checks for changes and the full CTest suite for behavior changes before committing. Documentation-only changes need a diff/whitespace review, not a rebuild.
- This checkout is in a cloud-managed macOS Desktop folder. If build or file reads stall, use a build directory outside Desktop, such as a permitted temporary directory. Source reads may still require cloud hydration.
- SocketCAN is Linux-only. Use console output and the host preview on macOS; verify physical CAN and `vcan` on Linux. Do not describe host tests as hardware validation.
- Update documentation and handoff/backlog notes when behavior, setup, or project status changes materially.

## Git workflow and user preference

- The user authorizes agents to make commits and push completed work when needed. After the relevant checks pass, review the diff, create a focused commit, and push it without asking for routine confirmation. A read-only task does not require a commit.
- Inspect the working tree and branch before editing or staging. Stage only changes belonging to the current task; preserve unrelated existing work and do not include it in a commit merely because it is present.
- Use clear commit messages describing the resulting change. Keep work on the appropriate existing task branch; use the `codex/` prefix when a new branch is needed.
- Push to the configured remote and appropriate branch. Use normal fast-forward pushes; do not force-push, rewrite shared history, or discard changes without explicit authorization. If the remote has advanced, inspect and reconcile safely while preserving local work.
- If permissions, authentication, or branch rules prevent committing or pushing, report the exact blocker and what remains local. Do not claim a push succeeded without checking its result.
- In the final response, summarize the change and relevant validation, and report the commit and push status when work was committed.
