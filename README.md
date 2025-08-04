# kubo

Simple virtual machine i'm making for fun.

<img width="15%" alt="image" src="https://github.com/user-attachments/assets/ae84fc45-b053-4a26-ba7d-d54f92afb0ec" />

# Building

You will need a [C++23 compiler](https://github.com/llvm/llvm-project/releases) and [cmake](https://cmake.org/) installed.

```bash
python configure.py && python build.py
```

# Manual

## Header

The following header must always be present in every program.


| Address | Name | Value | Type | Bytes |
| ------- | - | ----- | ---- | ----- |
| 0x00 | Signature | This is a kubo program | String | 21 |
| 0x16 | Data Segment Start| [0x16, 0x800000) | Int32 | 4 |
| 0x1A | Code Segment Start | [0x1A, 0x800000) | Int32 | 4 |
| 0x1E | Entrypoint | [0x1E, 0x800000) | Int32 | 4 |


## Sections

The following two sections must be always present:

### .DATA

Where all static data resides. You must store string literals and constants in here.

### .CODE

Where the code resides.

## Instruction Set


| Instruction | Format | Sideffect |
|-------------|----------|-----------|
| PUSHA | Value Section | Pushes Value into Section|
| PUSHB | \[Section + Offset\] Section | Pushes \[Section + Offset\] into Section|
| POP | Section | Pops a value from Section |
| CALLA | Value | Jumps to Value, creates a new frame and starts execution |
| CALLB | Value | Creates a new frame, and invokes an intrisic operation Value |
| RET | N/A | Pop the current stack frame and returns to caller |
