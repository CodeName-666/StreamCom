# StreamCom

**Version 2.3.0 · Arduino / PlatformIO · MIT license**

StreamCom adds a small text-command interface to Arduino firmware. Define a
command name, the variables it controls and an optional callback. The library
receives the command, validates its parameters, updates the variables and then
calls your function.

For example, `PID=15;0.12;0.23` updates three configured values. `HELP` lists the
available commands. This makes StreamCom useful for device configuration,
commissioning and interactive diagnostics over a serial terminal or another
Arduino `Stream` transport.

The same command definitions can be used with `HardwareSerial`, a Telnet stream
or another class derived from `Stream`. Transport setup, network connections and
application behavior remain under your control.

- Typed parameters: signed integers, floating-point values and Arduino strings.
- Optional callbacks, including commands without parameters.
- Independent stream instances with their own service lists and default commands.
- Incremental input reception with support for fragmented input and multiple lines.
- Parameter validation before application values change or callbacks run.
- Services can be added and removed at runtime.

## Installation

To use this checkout in another PlatformIO project, reference its local path:

```ini
[env:controller]
platform = espressif32
board = nodemcu-32s
framework = arduino
lib_deps = symlink://../StreamCom
```

Adjust the path to the library directory. The included example already references
this checkout. ArduinoSTL `^1.3.3` is resolved automatically for classic AVR;
ESP32 and ESP8266 use the standard library provided by their Arduino toolchains.

The version in `library.json` describes this checkout. Updating that file does
not publish a PlatformIO Registry release.

## Quick start

This complete example exposes `SET_I` and `PID` over Serial at 115200 baud.
Default commands are enabled automatically.

```cpp
#include <Arduino.h>
#include <StreamCom.h>

int32_t integerSetting = 0;
int32_t proportionalGain = 0;
float integralGain = 0.0f;
float derivativeGain = 0.0f;

void printPidValues(Stream* stream, void* callbackArguments, uint32_t parameterCount)
{
    if ((stream != nullptr) && (callbackArguments != nullptr) && (parameterCount == 3U))
    {
        (void)stream->print("PID: ");
        (void)stream->print(STREAMCOM_GET_VALUE(int32_t, callbackArguments, 0U));
        (void)stream->print(';');
        (void)stream->print(STREAMCOM_GET_VALUE(float, callbackArguments, 1U));
        (void)stream->print(';');
        (void)stream->println(STREAMCOM_GET_VALUE(float, callbackArguments, 2U));
    }
}

Service_t services[] = {
    {"SET_I", {&integerSetting}, {I32}, 1U, nullptr},
    {"PID", {&proportionalGain, &integralGain, &derivativeGain},
            {I32, F, F}, 3U, printPidValues}
};

const uint16_t SERVICE_COUNT =
    static_cast<uint16_t>(sizeof(services) / sizeof(services[0]));
StreamCom serialCommands;

void setup()
{
    Serial.begin(115200);
    serialCommands.init(Serial, services, SERVICE_COUNT);
}

void loop()
{
    serialCommands.loop();
    // Other application work can run here.
}
```

Send each command with a CR, LF or CRLF line ending:

| Input | Result |
|---|---|
| `SET_I=42` | Sets `integerSetting` to 42; no callback is required. |
| `PID=15;0.12;0.23` | Updates all three gains and prints them through the callback. |
| `HELP` | Lists this instance's commands and parameter types. |
| `SIZE` | Reports five services: two application commands plus three defaults. |
| `RESET` | Restarts the microcontroller on a supported platform. |

## Command format and validation

The default format is `COMMAND=value1;value2;value3`. Names are case-sensitive.
Only the first `=` separates the command from its parameters, so a `STR` field
can contain `=`. Parameter separators cannot be escaped or quoted.

A command must provide exactly its configured number of nonempty fields.
Missing fields, additional fields, malformed numbers, trailing numeric junk,
out-of-range values, numeric underflow, NaN and infinity are rejected. No target
values are changed and no callback runs when parameter validation fails.

For example, `PID=15`, `PID=15;;0.23` and `PID=invalid;0.12;0.23` are rejected.
Zero-parameter commands accept both `HELP` and `HELP=`; `HELP=extra` is rejected.
Outer command whitespace and numeric-field whitespace are trimmed.

`loop()` consumes at most 64 bytes per call without waiting for more input.
It preserves incomplete input between calls and handles separate command lines
individually. Output writes and user callbacks can still block.

For compatibility, commands without a line ending are processed after 1000 ms
without another received byte. A pause in a partially typed command can therefore
complete it. Set `STREAM_COM_IDLE_TIMEOUT_MS=0` to require explicit line endings.

The receive buffer holds 128 bytes by default, including the terminating NUL:
that allows 127 command bytes, excluding the line ending. Oversized or
NUL-containing records are discarded through the next line ending or enabled
idle boundary; their suffix is never executed as a new command.

## Service definitions and parameter types

A `Service_t` contains these fields, in this order:

| Field | Meaning |
|---|---|
| `token` | Unique command name without whitespace or command separators. |
| `params` | Pointers to the target variables or application context. |
| `paramTypes` | The matching type of each active parameter. |
| `nParams` | Number of expected fields; must not exceed `STREAM_COM_MAX_PARAMETER`. |
| `callback` | Optional function called after successful parameter conversion. |

| Type | Target | Accepted value / behavior |
|---|---|---|
| `I8` | `int8_t*` | Signed decimal integer, −128 to 127. |
| `I16` | `int16_t*` | Signed decimal integer, −32768 to 32767. |
| `I32` | `int32_t*` | Full signed 32-bit decimal range. |
| `I64` | `int64_t*` | Full signed 64-bit decimal range, including on AVR. |
| `F` | `float*` | Finite value within the supported float range. |
| `D` | `double*` | Finite value within the platform's double range. |
| `STR` | `String*` | Nonempty text; the target is an Arduino `String`, not a `char*`. |
| `RAW` | Application pointer | Consumes a field but leaves the pointed-to object unchanged. |
| `NONE` | Unused slot | Not valid as an active parameter inside `nParams`. |

Target objects must match their declared types. In particular, `I32` expects
`int32_t`, not `uint32_t`. Unused array slots may be left value-initialized.

The callback signature is unchanged:

```cpp
void commandCallback(Stream* stream, void* callbackArguments, uint32_t parameterCount);
```

`stream` is the transport that received the command. `callbackArguments` points
to the configured array of target pointers, not to the received text. Read a
value with `STREAMCOM_GET_VALUE(Type, callbackArguments, index)` or get its
pointer with `STREAMCOM_GET_PTR(Type, callbackArguments, index)`. The index must
be less than `parameterCount`, and `Type` must match the target object.

## Default commands and multiple instances

| Command | Behavior |
|---|---|
| `HELP` | Prints the receiving instance's registered services and parameter types. |
| `SIZE` | Reports that instance's service count, including enabled defaults. |
| `RESET` | Resets the MCU using the platform's reset mechanism. |

Each instance has its own registry. There is no global current-instance pointer.
Initialize each transport separately and call each instance's `loop()` regularly.
The transport itself must be initialized first; StreamCom does not establish
Wi-Fi or Telnet connections.

RESET supports ESP32, ESP8266 and classic AVR. AVR uses a watchdog reset and
requires a watchdog-compatible bootloader. A manually integrated unsupported
architecture receives an explicit unsupported-reset message.

Disable all defaults with `STREAM_COM_DEFAULT_LIST_ENABLE=0`, or remove one
from an instance with `deleteService("RESET")`. A removed name may then be
registered as an application command. `NUM` is not a built-in command.

`PING` and `VERSION` were evaluated as optional diagnostic extensions. They are
not required for the current interface and are not included in 2.3.0. Device
status, uptime and persistent settings can be implemented as application services.

## Configuration

Set overrides in PlatformIO `build_flags` so all translation units use the same
configuration; defining a macro only in `main.cpp` does not configure the library.

| Macro | Default | Purpose |
|---|---|---|
| `STREAM_COM_DEFAULT_LIST_ENABLE` | `true` | Enable `RESET`, `HELP` and `SIZE`. |
| `STREAM_COM_MAX_PARAMETER` | `4` | Maximum fields per service; range 1–255. |
| `STREAM_COM_CDM_DELIMITER` | `"="` | Command separator character set; legacy spelling retained. |
| `STREAM_COM_PARAM_DELIMITER` | `";"` | Parameter separator character set. |
| `STREAM_COM_RX_BUFFER_SIZE` | `128` | Receive storage including NUL; range 2–65535. |
| `STREAM_COM_IDLE_TIMEOUT_MS` | `1000` | Idle completion in ms; `0` requires CR/LF. |

Delimiter sets must be nonempty. Each character in a set is a separator; the
macros do not define multi-character separator sequences.

```ini
build_flags =
    -DSTREAM_COM_RX_BUFFER_SIZE=256
    -DSTREAM_COM_IDLE_TIMEOUT_MS=0
    -DSTREAM_COM_DEFAULT_LIST_ENABLE=0
```

Larger receive buffers and parameter arrays require more memory per instance.

## Service lifetime and runtime behavior

- `init()` replaces previous registrations, restores enabled defaults and clears
  pending input. Invalid descriptors and duplicate names are rejected.
- `addService()` borrows the descriptor; it does not copy it. The descriptor,
  token, stream and target objects must remain alive while in use. Keep descriptor
  fields and token strings unchanged while registered; target values may change.
- `deleteService()` accepts a name or a zero-based index. Unknown names and invalid
  indices are ignored. Removing a service does not delete its target objects.
- The registry supports up to 65535 services, including defaults, subject to RAM.
- Callbacks may add/remove services. Recursive `loop()` calls on the same instance
  are ignored. Serialize access from tasks and to targets shared between instances.

The existing `std::vector` and Arduino `String` implementation is retained.
StreamCom allocates memory and is not ISR-safe or a hard-real-time component.
String capacity is checked before committing converted values; do not rely on
recovery from vector allocation failure on exception-free targets. `float` and
`double` remain available on platforms without an FPU and may use software arithmetic.

Internal names follow the embedded C/C++ conventions: descriptive lowerCamelCase,
`m_` prefixes for members and `is`/`has`/`should` prefixes for boolean state. Local
variables are initialized at the beginning of functions or short-lived loop bodies.
Public API names and the existing file structure are preserved for compatibility;
configuration and public types therefore remain in `StreamCom.h`.

## Supported platforms and example builds

The package targets the Arduino framework on classic AVR, ESP32 and ESP8266.
The Serial example uses the local checkout and can be built from the repository root:

```sh
pio run -d examples -e nodemcu-32s
pio run -d examples -e atmega328
pio run -d examples -e nodemcuv2
```

The 2.3.0 implementation was checked with builds for these three boards and 56
host regression-test executions across four configurations. The host checks
covered parsing, validation, multiple instances, registry updates, 300 services,
input limits and timer rollover. They were run outside the repository; no
permanent host-test suite is included. Hardware behavior has not been verified
on physical boards in this revision.

## Changes in 2.3.0

- Preserved `Service_t`, the callback signature, access macros and service-management API.
- Replaced blocking input reception with bounded incremental processing.
- Added exact field-count checks and numeric validation before target updates.
- Fixed full-range `I64` parsing and callback suppression on invalid input.
- Made default commands instance-specific and removed global `mThis` state.
- Fixed reinitialization, duplicate registration and indices above 255 services.
- Fixed builds with disabled defaults or reduced parameter limits.
- Corrected callback pointer access, the Serial example and platform declarations.

When upgrading from 2.2.0, check command lengths against the receive-buffer limit
and ensure senders provide the exact parameter count. Previously tolerated malformed
input is now rejected. Prefer line endings; no-newline clients retain the default
idle-completion behavior. Internal helpers such as global `mThis` are no longer available.

## License

MIT. See [LICENSE](LICENSE).
