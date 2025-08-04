# kubo

Simple virtual machine i'm making for fun.

<img width="15%" alt="image" src="https://github.com/user-attachments/assets/ae84fc45-b053-4a26-ba7d-d54f92afb0ec" />

# Building

You will need a [C++23 compiler](https://github.com/llvm/llvm-project/releases) and [cmake](https://cmake.org/) installed.

```bash
python configure.py && python build.py
```

# Manual

## Instructions

| Code | Name | Arg1 | Arg2 | Description |
|- | ----------- | ---- | ---- | --------- |
| 0x00 | PUSHA | Value | Section | Pushes Arg1 into Arg2 |
| 0x01 | PUSHB | \[Section + Offset\] | Section | Pushes the value present in Arg1 into Arg2 |
| 0x02 | POP | Section | N/A | Pops a value from the given section |
| 0x03 | CALLA | Value | N/A | Starts executing the address Arg1 |
| 0x04 | CALLB | Value | N/A | Invokes an intrisic operation Arg1 |
| 0x05 | RET | N/A | N/A | Returns to caller |

## Sections

| Code | Name | Description |
| ------- | ----- | --------- |
| 0x00 | ARGUMENT | Values that will be passed to another function |
| 0x01  | SCOPE | Values that resides in the scope of the current function |
| 0x02  | DATA | Values that resides in the .DATA segment |

## Intrinsics

| Code | Name | Description |
| ------- | ----- | --------- |
| 0x00 | PRINT | Prints characters into the stdout |
| 0x01  | PRINTLN | Prints characters into the stdout with a newline |

## Header

The following header must always be present in every program.

| Address | Name | Value | Type | Bytes |
| ------- | - | ----- | ---- | ----- |
| 0x00 | Signature | This is a kubo program | String | 21 |
| 0x16 | Data Segment Start| 0x00 <= x < 0x800000 | Int32 | 4 |
| 0x1A | Code Segment Start | Data Segment Start < x < 0x800000 | Int32 | 4 |
| 0x1E | Entrypoint | Code Segment Start < x < 0x800000 | Int32 | 4 |

## Example

```
.DATA

13 Hello, World!

.CODE

ENTRYPOINT

PUSHB [.DATA + 0] ARGUMENT
CALLB println
POP ARGUMENT
RET
```
