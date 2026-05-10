# xps

A modern C++23 client library for [Newport XPS Unified motion controllers](https://www.newport.com/), implementing the XPS Unified TCP/IP protocol from scratch.

This is a personal project. It targets the base firmware tier and exposes an object-oriented API around `Controller`, `Group`, and `Positioner`.

## Status

🚧 **Work in progress.** Not all base-firmware functions are wrapped yet — see [Coverage](#coverage).

## Requirements

- C++23 compiler (Clang 17+, GCC 13+, or MSVC 19.40+ / VS 2022 17.10+)
- CMake 4.0+
- An XPS Unified controller reachable over TCP (default port 5001)

The library has no external runtime dependencies — only the standard library and the host's BSD/Winsock socket API.

## Building

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

To run the codec unit tests (no controller required):

```bash
ctest --test-dir build
```

## Quick start

```cpp
#include <xps/controller.hpp>
#include <print>

int main() {
    auto ctrl = xps::Controller::open("192.168.254.254");
    if (!ctrl) {
        std::println(stderr, "open failed: {}", ctrl.error().message);
        return 1;
    }

    auto version = ctrl->firmware_version();
    if (version) std::println("XPS firmware: {}", *version);

    auto xy = ctrl->group("XY", /*axis_count=*/2);

    auto result = xy.initialize()
        .and_then([&] { return xy.home_search(); })
        .and_then([&] { return xy.move_absolute({10.0, 5.0}); });

    if (!result) {
        std::println(stderr, "motion failed (code {}): {}",
                     result.error().code, result.error().message);
        return 1;
    }

    if (auto pos = xy.position_current()) {
        std::println("at: ({}, {})", (*pos)[0], (*pos)[1]);
    }
}
```

See [`examples/`](examples/) for more.

## Design

### Layered architecture

```
┌─────────────────────────────────┐
│  Controller / Group / Positioner│   Layer 4: domain
├─────────────────────────────────┤
│  command formatter / parser     │   Layer 3: wire codec
├─────────────────────────────────┤
│  Connection (request/response)  │   Layer 2: framed messaging
├─────────────────────────────────┤
│  raw socket (BSD / Winsock)     │   Layer 1: transport
└─────────────────────────────────┘
```

Only Layer 4 is in the public headers. Lower layers live under `src/internal/`.

### Error handling

Every fallible operation returns `xps::Result<T>` (a `std::expected<T, xps::Error>`). No exceptions are thrown for protocol-level errors — only for genuine programmer errors (e.g. constructing a `Group` with a name containing whitespace, which would produce malformed wire commands).

```cpp
struct Error {
    int code;            // XPS error code (0 = success, < 0 = error)
    std::string message; // human-readable description
};
```

All Result-returning methods are `[[nodiscard]]`.

### Wire protocol

The XPS Unified protocol is a textual request/response protocol over TCP. This library implements it directly:

- Commands are formatted as `FunctionName(arg1,arg2,...)` with no spaces.
- Output slots are encoded with placeholder tokens (`double *`, `int *`, `char *`).
- Each command is terminated with `,EndOfAPI.`.
- Replies start with the integer return code, followed by comma-separated outputs, and end with `,EndOfAPI.`.
- Floating-point values use `.` as the decimal separator regardless of host locale (via `std::format`).

A single `Connection` serializes commands across threads with an internal mutex; if you need concurrent operations (e.g. status polling during a long move, or aborts), open a second `Connection`.

### Group / positioner discovery

The XPS controller does not expose an API to enumerate groups or positioners — these are defined statically in the controller's `system.ini` file and known only to the controller. You must tell this library which groups and positioners exist by name when constructing handles:

```cpp
auto xy   = ctrl->group("XY", 2);
auto xy_x = ctrl->positioner("XY.X");
```

A handle is just a name + connection reference — no validation happens at construction time. The first call against a non-existent name will return `Error{-19, "Group name doesn't exist or unknown command"}`.

## Coverage

This library targets the **base firmware tier only**. Functions marked `[Extended]` or `[MODULE]` in the XPS Unified Programmer's Manual are out of scope.

Implemented so far:

- [x] `Controller` — connect, firmware version, controller status, error string lookup
- [x] `Group` — initialize, home search, move absolute / relative, abort, kill, status, position get
- [ ] `Group` — jog, spin, referencing, analog tracking, external profiler
- [ ] `Positioner` — corrector parameters (PID / PIDFF variants), backlash, stage parameters, encoder
- [ ] XY / XYZ / TZ / Hexapod / Spindle group-type-specific calls

## Project layout

```
xps/
├── include/newport/xps/        # public headers
│   ├── controller.hpp
│   ├── group.hpp
│   ├── positioner.hpp
│   ├── error.hpp
│   ├── status.hpp      # GroupStatus, ControllerStatus enums (manual §8)
│   └── parameters.hpp  # PIDFFAccelerationParameters, etc.
├── src/
│   ├── controller.cpp
│   ├── group.cpp
│   ├── positioner.cpp
│   └── internal/
│       ├── socket.{hpp,cpp}      # Layer 1
│       ├── connection.{hpp,cpp}  # Layer 2
│       └── codec.{hpp,cpp}       # Layer 3
├── tests/
│   └── codec_test.cpp
├── examples/
│   └── home_and_move.cpp
└── CMakeLists.txt
```

## References

- Newport XPS Unified Programmer's Manual (EDH0373En1046, 09/25)

## License

TBD.