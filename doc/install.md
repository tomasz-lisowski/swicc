# Install

## Make Targets
- `main`: This builds a static library without any debug information or debug utilities.
- `main-dbg`: This builds a static library with all debug information and debug utilities.
- `test`: Build the testing binary and link it with the non-debug version of the swICC library.
- `test-dbg`: Build the testing binary with debug information and an address sanitizer. and link it with the non-debug version of the swICC library.
- `clean`: Performs a cleanup of the project and all sub-modules.

If you would like to compile the library with extra compiler flags, use the `ARG` variable when calling `make`, e.g.,
```make main-dbg ARG="-DDEBUG_CLR -DEXAMPLE_DEFINE -fsanitize=address"```

All possible values that can be added to `ARG`:
- `-DDEBUG_CLR` to add color to the debug output.
- `-DDEBUG_NET_MSG` to parse received network messages and print them.
- `-DTRACE_PYSIM` to output traces of APDU messages on stdout.
