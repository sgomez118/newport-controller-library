# newport-controller-library

A modern C++23 shared library for controlling Newport XPS-D motion controllers over TCP/IP.

Newport provides a .NET assembly and legacy C drivers for the XPS. This library offers a clean, dependency-free native C++ alternative with value-semantic handles, structured error reporting via `std::expected`, and a straightforward threading model.

## Features

- **No exceptions** — all fallible operations return `std::expected<T, XpsError>`
- **Rich errors** — every `XpsError` carries the XPS error code, the raw command string, and a human-readable message
- **Automatic reconnection** — exponential backoff with user-supplied callbacks for jog-mode safety
- **Simple threading model** — one `XpsController` = one socket = one thread; create multiple instances for concurrency (the XPS-D supports up to 100 simultaneous connections)
- **Value-semantic handles** — `Group` and `Positioner` are lightweight and cheap to copy

## Requirements

| Tool | Version |
|------|---------|
| C++ compiler | MSVC 2022+ or GCC 13+ or Clang 17+ |
| CMake | 4.0+ |
| Ninja | any recent |
| vcpkg | any recent |

## Building

1. Set the `VCPKG_ROOT` environment variable to your vcpkg installation directory.

2. Configure and build:

```sh
cmake --preset x64-debug
cmake --build build
```

3. Run the tests:

```sh
ctest --test-dir build -C Debug --output-on-failure
```

The `CMakePresets.json` defines a `base` preset (Ninja generator, `build/` output directory, vcpkg toolchain). Create a `CMakeUserPresets.json` (git-ignored) that inherits from `base` and sets `VCPKG_ROOT` if you prefer not to use an environment variable:

```json
{
  "version": 10,
  "configurePresets": [
    {
      "name": "x64-debug",
      "inherits": "base",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" },
      "environment": { "VCPKG_ROOT": "C:/path/to/vcpkg" }
    }
  ]
}
```

## Reference manual

The XPS-D Unified Programmer's Manual is available from Newport/MKS Instruments. It is not included in this repository. You will need it to understand the command set and error codes when contributing to the protocol layer.

## API overview

```cpp
#include "newport/xps/xps_controller.h"

using namespace newport::xps;

ConnectionConfig cfg;
cfg.host     = "192.168.0.254";
cfg.username = "Administrator";
cfg.password = "Administrator";

XpsController controller(cfg);

controller.SetOnReconnect([] { /* restart jog if needed */ });

if (auto result = controller.Connect(); !result) {
    auto& err = result.error();
    // err.code, err.command, err.message
    return;
}

// Multi-axis group
Group stage = controller.GetGroup("XYZR");
stage.Initialize();
stage.HomeSearch();
stage.MoveAbsolute({0.0, 0.0, 0.0, 0.0});

// Single-axis positioner
Positioner x = stage.GetPositioner("X");  // full name: "XYZR.X"
x.MoveRelative(1.5);
```

### Types

| Type | Description |
|------|-------------|
| `XpsController` | Owns the TCP connection. Not copyable; create one per thread. |
| `Group` | Handle to a named XPS group. Lightweight value type. |
| `Positioner` | Handle to a single axis or single-axis group. Lightweight value type. |
| `XpsError` | Error detail: `int code`, `std::string command`, `std::string message`. |
| `ConnectionConfig` | TCP connection parameters, credentials, and timeout settings. |

`Group` and `Positioner` are non-owning — the `XpsController` that produced them must outlive them.

## Project status

| Phase | Scope | Status |
|-------|-------|--------|
| 0 — Skeleton | CMake shared library, vcpkg + GTest wired, all public headers with full signatures, stub `.cc` files compile clean | Done |
| 1a — Transport | `TcpSocket`: connect with timeout, send, `ReadUntil` sentinel, drop detection; `MockXpsServer` scripted TCP responder; 8 unit tests | Done |
| 1b — Protocol | `XpsProtocol`: command formatting, `EndOfAPI` parser, error code table | Planned |
| 1c — Connection | `XpsController::Connect` / `Disconnect`, reconnect loop, callbacks | Planned |
| 1d — Motion | `Group` and `Positioner` motion and jog methods | Planned |
| 1e — Integration | Full round-trip tests gated on `XPS_INTEGRATION_HOST` env var | Planned |

## Namespace

All public symbols live in `newport::xps`. The `newport` namespace is intentionally kept open for future Newport product libraries.

## Code style

This project follows the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html): PascalCase for types and functions, snake_case for variables and fields, `.cc` source extension, and `#ifndef` include guards.
