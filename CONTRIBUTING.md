# Contributing to Hotel Room Thermostat

Thank you for your interest in contributing! This document provides guidelines for contributing to the project.

## 🚀 Getting Started

1. **Fork the repository**
2. **Clone your fork**: `git clone https://github.com/YOUR_USERNAME/HotelRoomThermostat.git`
3. **Create a branch**: `git checkout -b feature/your-feature-name`
4. **Make your changes**
5. **Test thoroughly** (see Testing section below)
6. **Commit**: `git commit -m "Add: brief description"`
7. **Push**: `git push origin feature/your-feature-name`
8. **Create Pull Request**

## 📋 Development Guidelines

### Code Style
- Follow existing code conventions (K&R style for C/C++)
- Use 4 spaces for indentation (no tabs)
- Comment complex logic, especially HVAC control
- Keep functions focused (single responsibility)
- Use descriptive variable names

### Commit Messages
- Use imperative mood: "Add feature" not "Added feature"
- First line: brief summary (50 chars max)
- Blank line, then detailed description if needed
- Reference issues: "Fix #123: window sensor bug"

### Critical Rules
- **NEVER** add `Serial.print()` in Modbus callbacks (`cb_hreg_write`)
- **NEVER** change pin assignments without hardware verification
- **ALWAYS** maintain 100ms relay deadband delay
- **ALWAYS** test Modbus communication after any serial code changes

## 🧪 Testing Requirements

Before submitting a PR, ensure:

1. **Build succeeds**: `platformio run`
2. **Upload succeeds**: `platformio run --target upload`
3. **Modbus tests pass**: All 3 test scripts in `/test` directory
   ```bash
   python test/quick_test.py
   python test/run_test.py
   ```
4. **UI navigation works**: Test all swipe panels and button actions
5. **Settings persist**: Change values, reboot, verify NVS storage
6. **Window sensor override**: Test HVAC lock when window open

## 📝 Documentation Updates

If your change affects:
- **Register map**: Update `README.md` Modbus section
- **Hardware pins**: Update `ESP32_4848S040.h` and pin table
- **UI flow**: Update `docs/DIAGRAMS.md`
- **API changes**: Update `include/*.h` header comments
- **New features**: Add section to `README.md`

Keep `/docs/FSD.md` synchronized with code changes.

## 🐛 Bug Reports

Use GitHub Issues with:
- Clear title describing the problem
- Steps to reproduce
- Expected vs. actual behavior
- Serial monitor output (if relevant)
- Modbus test results (if communication issue)
- Platform info (board, PlatformIO version)

## 💡 Feature Requests

Before proposing features:
1. Check existing issues/PRs
2. Ensure it fits project scope (hotel HVAC control)
3. Consider hardware limitations (GPIO, RAM, flash)
4. Describe use case and benefit

## 🔍 Pull Request Review Criteria

Your PR will be reviewed for:
- ✅ Code quality and style
- ✅ Test coverage
- ✅ Documentation completeness
- ✅ Backward compatibility (if applicable)
- ✅ No breaking changes to Modbus API
- ✅ Performance impact (RAM/Flash usage)

## 🙏 Areas Needing Help

Priority contributions welcome:
- [ ] Touch calibration routine
- [ ] Multi-language support (i18n)
- [ ] MQTT/HTTP weather integration
- [ ] Energy monitoring features
- [ ] Unit tests (Google Test framework)
- [ ] CI/CD pipeline (GitHub Actions)

## 📞 Questions?

- Open a GitHub Discussion for general questions
- Use Issues for bugs/feature requests
- Check `/docs` folder for technical details

## 🎯 Code of Conduct

Be respectful, constructive, and collaborative. We're here to build great embedded systems together!

---

**Thank you for contributing!** 🚀
