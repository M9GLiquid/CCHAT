# Git Commit Message Guidelines

**Cursor agents:** This repo includes a skill that encodes the same rules for
automated commits: `.cursor/skills/git-commit-messages/SKILL.md`.

This document outlines best practices for writing clear, consistent commit messages based on [Chris Beams' seven rules](http://chris.beams.io/posts/git-commit/#seven-rules).

## The Seven Rules

1. **Separate subject from body with a blank line**
2. **Limit the subject line to 50 characters**
3. **Capitalize the subject line**
4. **Do not end the subject line with a period**
5. **Use the imperative mood in the subject line**
6. **Wrap the body at 72 characters**
7. **Use the body to explain what and why vs. how**

## Commit Message Format

```
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

- **Scope** (Recommended): The area of the codebase affected (e.g., `unity`, `backend`, `frontend`, `api`)

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

### Footer (Optional)

- Reference issues: `Fixes #123`, `Closes #456`
- Breaking changes: `BREAKING CHANGE: <description>`

## Examples

### Good Commit Messages

```
feat(unity): Add motion playback system

Implement a system to play back generated motion sequences
in Unity. This allows users to preview animations before
applying them to characters.

The system uses a queue-based approach to handle multiple
motion sequences and provides callbacks for playback events.
```

```
fix(backend): Resolve ZeroMQ connection timeout

The ZeroMQ connection was timing out after 5 seconds due
to incorrect socket configuration. This caused the backend
to fail when Unity attempted to connect.

Updated socket options to use ZMQ_LINGER=0 and increased
connection timeout to 30 seconds.
```

```
docs: Add commit message guidelines

Create comprehensive commit message template based on
Chris Beams' seven rules to ensure consistent commit
history across the project.
```

### Bad Commit Messages

```
fixed bug  (# Avoid: past tense, no type prefix)
```

```
Add feature to play motion (# Avoid: too vague, no context)
```

```
feat: Added new system for playing back motion sequences in Unity (# Avoid: past tense, too long)
```

## Additional Resources

- [How to Write a Git Commit Message](http://chris.beams.io/posts/git-commit/#seven-rules) - Chris Beams
- [A Note About Git Commit Messages](http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html) - Tim Pope
- [Git SCM - Commit Guidelines](http://www.git-scm.com/book/en/Distributed-Git-Contributing-to-a-Project#Commit-Guidelines)
- [Linux Kernel Commit Message Guidelines](https://github.com/torvalds/subsurface/blob/master/README#L82-109)
- [On Commit Messages](http://who-t.blogspot.co.at/2009/12/on-commit-messages.html)
- [Erlang OTP Commit Message Guidelines](https://github.com/erlang/otp/wiki/writing-good-commit-messages)
- [Spring Framework Commit Guidelines](https://github.com/spring-projects/spring-framework/blob/30bce7/CONTRIBUTING.md#format-commit-messages)

## Quick Reference Template

```
<type>(<scope>): <subject>

<body explaining what and why>

<footer with issue references>
```

## More Examples

```
feat(module): Add lorem ipsum dolor sit amet

Implement consectetur adipiscing elit functionality
to improve sed do eiusmod tempor incididunt. This
change enables ut labore et dolore magna aliqua.

The implementation follows enim ad minim veniam
patterns and maintains quis nostrud exercitation
compatibility.
```

```
fix(component): Resolve ullamco laboris nisi issue

The system was experiencing ut aliquip ex ea commodo
consequat errors when processing duis aute irure
dolor data. This occurred because reprehenderit in
voluptate validation was missing.

Added velit esse cillum dolore check before accessing
eu fugiat nulla pariatur data and return early with
error message if invalid.
```

```
docs: Update excepteur sint occaecat cupidatat

Add missing documentation for non proident sunt in
culpa qui officia deserunt mollit anim id est
laborum section. Include request/response examples
and error codes.

This helps new developers understand the API
structure and integration points.
```
