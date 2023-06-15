# tpm-send

tpm2-send is a no-dependency utility for sending raw bytes to the TPM.

Want to access the TPM 2.0 from within WSL2? Just compile tpm2-send.exe and then call it from your WSL2 shell.


# Usage

tpm2-send takes an input stream (by default `stdin`) and an output stream (by default `stdout`).

```cmd
tpm2-send [-i <input file>] [-o <output file>]
```

## Examples

```cmd
tpm2-send -i in.bin -o out.bin
```

Or you can read from `stdin` and write to `stdout`:

```cmd
REM cmd.exe:
type in.bin | tpm2-send
```

```bash
# bash:
printf "80010000000c0000017b0004" | xxd -r -p | ./tpm2-send.exe | xxd -p
```


# Build

## Windows
Make sure you have [Build Tools for Visual Studio 2022](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022) installed.

To compile:

```cmd
REM setup environment
'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat'

REM compile
cl /W4 /link tpm2-send.c tbs.lib
```

## Linux

Linux is not supported, yet. Patches welcome!
