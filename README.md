> [!WARNING]
> This project was superseded by [tpm2-software/tpm2-send-tbs](https://github.com/tpm2-software/tpm2-send-tbs).

# tpm2-send-tbs

tpm2-send-tbs is a no-dependency utility for sending raw bytes to the TPM.

Want to access the TPM 2.0 from within WSL2? Just compile `tpm2-send-tbs.exe` and then call it from your WSL2 shell.


# Usage

tpm2-send-tbs takes an input stream (by default `stdin`) and an output stream (by default `stdout`).

```cmd
tpm2-send-tbs [-x] [-i <input file>] [-o <output file>]
```

## Examples

By default, tpm2-send-tbs reads from `stdin` and writes to `stdout`:

```cmd
REM cmd.exe:
type in.bin | tpm2-send-tbs
```

```bash
# bash:
printf "80010000000c0000017b0004" | xxd -r -p | ./tpm2-send-tbs.exe | xxd -p
```

You can use `--hex` to switch to hex stream format.

```bash
# bash:
printf "80010000000c0000017b0004" | ./tpm2-send-tbs.exe --hex
```

### Note:

`xxd` buffers until its input pipe is closed. With many processes, commands/responses are a back and forth. E.g. tcti-cmd waits for a TPM response before sending the next command.  Thus, `xxd` would block indefinitely, here. You can use `./hex` and `./unhex` instead.

```bash
# bash:
tpm2_getrandom -T "cmd: ./hex | ./tpm2-send-tbs.exe --hex | ./unhex" --hex 4
```

### Note:

The WSL2 pipe from linux processes to windows processes is broken. It turns LF into CR+LF, even if opened in bytewise mode. As a result `tpm2_getrandom -T "cmd: ./tpm2-send-tbs.exe" --hex 4` will not work.

### Note:

The WSL2 pipe from windows processes to linux  processes seems to be broken. The receive call (linux side) is blocking although all bytes are sent already. Only once one additional byte is sent, the previous bytes (without the additional byte) are received.

# Build

## Windows
Make sure you have [Build Tools for Visual Studio 2022](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022) installed.

To compile:

```cmd
REM setup environment
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat"

REM compile
cl /W4 tpm2-send-tbs.c /link tbs.lib
```

## Utilities
The utilities `hex` and `unhex` are compiled in WSL2:

```bash
gcc hex.c -o hex
gcc unhex.c -o unhex
```

