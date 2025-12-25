# Contributing to WeaR-emu

Thank you for your interest in contributing to WeaR-emu! This document outlines our guidelines and best practices.

---

## Getting Started

1. **Fork** the repository on GitHub
2. **Clone** your fork locally
3. **Create** a feature branch
4. **Make** your changes
5. **Test** thoroughly
6. **Submit** a pull request

---

## Code Style

### C++ Guidelines

We follow modern C++23 best practices:

```cpp
// ✅ Good
[[nodiscard]] std::expected<Result, Error> performOperation() {
    if (auto result = tryOperation(); result) {
        return *result;
    }
    return std::unexpected("Operation failed");
}

// ❌ Avoid
Result performOperation() {
    Result r;
    if (tryOperation(&r)) return r;
    throw std::runtime_error("fail");
}
```

### Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| Classes | `PascalCase` with `WeaR_` prefix | `WeaR_RenderEngine` |
| Functions | `camelCase` | `renderFrame()` |
| Member Variables | `m_camelCase` | `m_initialized` |
| Constants | `UPPER_SNAKE_CASE` | `MAX_FRAMES_IN_FLIGHT` |
| Namespaces | `PascalCase` | `WeaR::Graphics` |

### File Organization

```
src/
├── Core/           # CPU, Memory, System
├── Graphics/       # Vulkan, Rendering
├── HLE/            # High-Level Emulation
│   ├── Modules/    # libkernel, libGnm, etc.
│   └── FileSystem/ # VFS
├── GUI/            # Qt Interface
├── Input/          # Controller handling
└── Audio/          # Sound output
```

---

## Pull Request Process

### Before Submitting

- [ ] Code compiles without warnings (`-Wall -Wextra -Werror`)
- [ ] All existing tests pass
- [ ] New features have accompanying tests
- [ ] Documentation is updated if applicable
- [ ] Commit messages follow conventional format

### Commit Message Format

```
type(scope): brief description

Longer explanation if needed.

Fixes #123
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

Examples:
```
feat(cpu): implement SSE2 MOVDQA instruction
fix(render): correct swapchain recreation on resize
docs(readme): update build instructions for Qt 6.10
```

### PR Template

When opening a PR, please include:

```markdown
## Description
Brief description of changes.

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Testing
How was this tested?

## Screenshots
If applicable, add screenshots.

## Checklist
- [ ] Code follows project style
- [ ] Self-reviewed
- [ ] Commented complex sections
- [ ] Updated documentation
```

---

## Architecture Guidelines

### Adding New HLE Modules

1. Create `src/HLE/Modules/WeaR_LibXxx.cpp`
2. Define syscall handlers following existing patterns
3. Register handlers in `registerLibXxxHandlers()`
4. Add to syscall dispatcher

```cpp
namespace WeaR::LibXxx {

SyscallResult hle_sceXxxInit(WeaR_Context& ctx, WeaR_Memory& mem, ...) {
    // Implementation
    return SyscallResult{0, true, ""};
}

void registerLibXxxHandlers(WeaR_Syscalls& dispatcher) {
    dispatcher.registerHandler(SYSCALL_NUM, hle_sceXxxInit);
}

} // namespace WeaR::LibXxx
```

### Adding GPU Commands

1. Add opcode to `PM4::Opcode` enum
2. Implement handler in `WeaR_GnmDriver`
3. Create corresponding `WeaR_DrawCmd` type
4. Handle in `WeaR_RenderEngine::renderFrame()`

---

## Reporting Issues

### Bug Reports

Include:
- **WeaR-emu version** (commit hash)
- **OS and GPU** specs
- **Steps to reproduce**
- **Expected vs actual behavior**
- **Log output** if available

### Feature Requests

Include:
- **Use case** description
- **Proposed solution**
- **Alternatives considered**

---

## License

By contributing, you agree that your contributions will be licensed under the GNU General Public License v2.0.

---

## Thank You

Every contribution helps make WeaR-emu better. Whether it's a bug fix, new feature, documentation improvement, or simply testing and reporting issues — we appreciate your help!

---

<p align="center">
  <strong>Happy Contributing!</strong>
</p>
