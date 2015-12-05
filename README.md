cuppa
=====

Port of the OSX utility `caffeinate(8)` to Linux.

## Usage

```
Usage:
  cuppa [OPTION...] -- COMMAND [ARGS...] - prevent the system from sleeping

Help Options:
  -h, --help                  Show help options

Application Options:
  -d, --display-sleep         Prevent the display from sleeping.
  -i, --system-idle           Prevent the system from idle sleeping.
  -m, --disk-idle             Prevent the disk from idle sleeping.
  -s, --power-sleep           Prevent the system from sleeping if on AC power.
  -u, --user-active           Declare that user is active.
  -t, --timeout=T             Duration in seconds of the override, or 5 seconds if used as a flag.
  -w, --waitpid=PID           Wait for process completion.

This program relies heavily on D-Bus interfaces.
```
