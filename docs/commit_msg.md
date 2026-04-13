# Git Commit Message Guidelines

This document outlines best practices for writing clear, consistent commit
messages based on [Chris Beams' seven rules](http://chris.beams.io/posts/git-commit/#seven-rules),
with an added preference for short user-centric bodies.

## The Seven Rules

1. **Separate subject from body with a blank line**
2. **Limit the subject line to 50 characters**
3. **Capitalize the subject line**
4. **Do not end the subject line with a period**
5. **Use the imperative mood in the subject line**
6. **Wrap the body at 72 characters**
7. **Use the body to explain what and why vs. how**

## Commit Message Format

```text
<type>(<scope>): <subject>

<body>

<footer>
```

### Subject Line (Required)

- **Type**: Use one of the following prefixes:
  - `feat`: A new feature
  - `fix`: A bug fix
  - `docs`: Documentation only changes
  - `style`: Code style changes (formatting, missing semi-colons, etc.)
  - `refactor`: Code refactoring without bug fixes or features
  - `perf`: Performance improvements
  - `test`: Adding or updating tests
  - `chore`: Maintenance tasks, build config, etc.

- **Scope** (Recommended): The area of the codebase affected
  (e.g. `unity`, `backend`, `frontend`, `api`)

- **Subject**:
  - Maximum 50 characters
  - Capitalize first letter
  - No period at the end
  - Use imperative mood ("Add feature" not "Added feature" or "Adds feature")
  - Describe what the commit does, not what it did

### Body (Optional)

- Separate from subject with a blank line
- Wrap at 72 characters
- Explain **what** changed and **why**, not **how**
- Provide context and reasoning
- Reference issue numbers if applicable
- Prefer writing for the **user of the app** when the change is user-facing
- Keep the body concise and avoid low-level implementation details unless they
  are necessary to understand the impact

### Style Preference For This Repo

- Start each body paragraph with a clear action word such as `Added`,
  `Updated`, `Fixed`, `Improved`, or `Removed`
- Make the body to the point and centered on the outcome a user will notice
- If the change is mostly internal, describe the practical benefit rather than
  listing code-level edits
- One or two short paragraphs is usually enough

### Footer (Optional)

- Reference issues: `Fixes #123`, `Closes #456`
- Breaking changes: `BREAKING CHANGE: <description>`

## Examples

### Good Commit Messages

```text
feat(unity): Add motion playback system

Added motion playback so users can preview generated
animations before applying them to characters.

Updated the playback flow to handle multiple queued
sequences more reliably.
```

```text
fix(backend): Resolve ZeroMQ connection timeout

Fixed a backend connection issue that caused Unity requests
to time out after a few seconds.

Updated the socket configuration so connections stay alive
longer during startup.
```

```text
docs: Add commit message guidelines

Added commit message guidance so contributors can write more
consistent and readable project history.
```

### Bad Commit Messages

```text
fixed bug
```

Avoid: past tense, no type prefix, too vague.

```text
Add feature to play motion
```

Avoid: too vague, no useful context.

```text
feat: Added new system for playing back motion sequences in Unity
```

Avoid: past tense in the subject and too long.

```text
fix(client): Update socket helper

Changed send_all and refactored the socket loop to use a
different helper function and renamed local variables.
```

Avoid: starts with a weak action word and focuses on code
internals instead of the user-visible outcome.

## Additional Resources

- [How to Write a Git Commit Message](http://chris.beams.io/posts/git-commit/#seven-rules) - Chris Beams
- [A Note About Git Commit Messages](http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html) - Tim Pope
- [Git SCM - Commit Guidelines](http://www.git-scm.com/book/en/Distributed-Git-Contributing-to-a-Project#Commit-Guidelines)
- [Linux Kernel Commit Message Guidelines](https://github.com/torvalds/subsurface/blob/master/README#L82-109)
- [On Commit Messages](http://who-t.blogspot.co.at/2009/12/on-commit-messages.html)
- [Erlang OTP Commit Message Guidelines](https://github.com/erlang/otp/wiki/writing-good-commit-messages)
- [Spring Framework Commit Guidelines](https://github.com/spring-projects/spring-framework/blob/30bce7/CONTRIBUTING.md#format-commit-messages)

## Quick Reference Template

```text
<type>(<scope>): <subject>

Added|Updated|Fixed <short user-facing summary>

Added|Updated|Fixed <optional second short user-facing summary>

<footer with issue references>
```

## More Examples

```text
feat(module): Add export flow

Added an export flow so users can save generated data from
the app without using manual workarounds.

Updated the export behavior to produce clearer file names.
```

```text
fix(component): Resolve connection retry issue

Fixed a retry issue that caused the app to disconnect too
quickly when the server was briefly unavailable.
```

```text
docs: Update API usage guide

Updated the documentation with clearer request examples and
the main error cases users may run into during setup.
```
