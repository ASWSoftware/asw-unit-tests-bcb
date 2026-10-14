# Contributing to ASWUnitTestsBCB

Thanks for considering a contribution. This project is a small C++Builder unit-testing framework
that needs only the RTL and builds with the classic Win32 compiler, and contributions are expected
to keep it that way.

## Before you start

- For anything beyond a small fix, open an issue first to discuss the change. This avoids wasted
  work on something that doesn't fit the project's scope (see `AGENTS.md`).
- Coding standards, repository layout, and language/toolchain compatibility rules live in
  [AGENTS.md](AGENTS.md). Read it before making changes; it is the single source of truth for how
  code in this repository should look, and these guidelines don't repeat it.
- Base your branch on `develop`, not `main`. `main` tracks released versions; `develop` is where
  in-progress work is integrated before a release.

## Making a change

1. Identify the code path that owns the behavior you're changing.
2. Make the smallest change that addresses the issue. Don't bundle unrelated refactoring,
   formatting, or reordering into the same change.
3. Add or update focused tests for the behavior change, including error and compiler-specific cases
   where applicable.
4. Build with the classic Win32 compiler (bcc32) and the Win64x compiler (bcc64x), and run the
   unit-test executable from each build. See [AGENTS.md](AGENTS.md#building-and-testing).
5. Check compiler warnings and errors; a change should not introduce new ones.
6. If you touched compiler-specific code (`#if defined(__clang__)` or similar), also build and run
   with bcc32c.

## Formatting

This repository uses `uncrustify` for consistent formatting, enforced via a Git hook. Enable it
locally with:

```
git config --local core.hooksPath .githooks/
```

## Commit history

Each commit in a branch should focus on one specific task or fix, and the commits should be in a
logical order that tells the story of how the change was built. Before opening a PR, review your own
branch's commit history and confirm each commit actually does what its message says.

[TortoiseGit](https://tortoisegit.org/) (free) is a good tool for this: it makes editing a commit
message, reordering commits, and squashing an "oops, fix typo" commit into the commit it belongs
with straightforward.

Pull requests in this repository are merged with their commit history intact, not squashed, so a
clean history matters: it's what future readers use to see where a bug was introduced and the
reasoning behind a change.

Once a PR is open, one or two additional fixup commits are fine without revisiting earlier history.
Beyond that, edit or squash the fixups into the commits they belong with and push with
`git push --force-with-lease`.

## Submitting a pull request

- Target the `develop` branch.
- Keep the diff focused on the change described in your issue or PR description. Review your own
  diff for accidental file changes, ordering regressions, missing includes, and compiler
  assumptions before requesting review.
- Describe what changed and why, not just what the code now does.
- Don't include generated build output (`build/`, `Debug/` and `Release/` directories are already
  git-ignored).

## Reporting bugs

Open a GitHub issue with:

- The RAD Studio version, compiler and platform (e.g. RAD Studio 13.1, bcc32, Win32).
- The command line used to run the tests, if relevant.
- The expected behavior versus the actual behavior.
- A minimal reproduction if possible.
