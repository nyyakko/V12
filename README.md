# kubo

Simple virtual machine i'm making for fun.

<img width="15%" alt="image" src="https://github.com/user-attachments/assets/ae84fc45-b053-4a26-ba7d-d54f92afb0ec" />

# Building

You will need a [C++23 compiler](https://github.com/llvm/llvm-project/releases) and [cmake](https://cmake.org/) installed.

```bash
python configure.py && python build.py
```

# Manual

## Instruction Set


| Instruction | Arg1 | Arg2 | Sideffect |
|-------------|------|------|-----------|
| PUSHA | Value | Section | Pushes Arg1 into Arg2 |
| PUSHB | \[Section + Offset\] | Section | Pushes the value present in Arg1 into Arg2 |
| POP | Section | N/A | Pops a value from the given section |
| CALLA | Value | N/A | Starts executing the address Arg1 |
| CALLB | Value | N/A | Invokes an intrisic operation Arg1 |
| RET | N/A | N/A | Returns to caller |

## Header

The following header must always be present in every program.


| Address | Name | Value | Type | Bytes |
| ------- | - | ----- | ---- | ----- |
| 0x00 | Signature | This is a kubo program | String | 21 |
| 0x16 | Data Segment Start| 0x00 <= x < 0x800000 | Int32 | 4 |
| 0x1A | Code Segment Start | Data Segment Start < x < 0x800000 | Int32 | 4 |
| 0x1E | Entrypoint | Code Segment Start < x < 0x800000 | Int32 | 4 |

## Sections

The following two sections must be always present:

### .DATA

Where all static data resides. You must store string literals and constants in here.

### .CODE

Where the code resides.

