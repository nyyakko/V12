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

The following header should reside at address 0 of every valid program:

```c++
static constexpr std::string_view MAGIC = "This is a kubo program";

struct Header
{
    char magic[MAGIC.size()] {};

    int32_t dataSegmentStart;
    int32_t codeSegmentStart;
    int32_t entryPoint;
};
```

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
