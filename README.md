# kubo

Simple virtual machine i'm making for fun.

<img width="15%" alt="image" src="https://github.com/user-attachments/assets/ae84fc45-b053-4a26-ba7d-d54f92afb0ec" />

# Building

You will need a [C++23 compiler](https://github.com/llvm/llvm-project/releases) and [cmake](https://cmake.org/) installed.

```bash
python configure.py && python build.py
```

# Instruction Reference


| Instruction | Format | Sideffect |
|-------------|----------|-----------|
| PUSHA | Value Section | Pushes Value into Section|
| PUSHB | \[Section + Offset\] Section | Pushes \[Section + Offset\] into Section|
| POP | Section | Pops a value from Section |
| CALLA | Value | Jumps to Value, creates a new frame and starts execution |
| CALLB | Value | Creates a new frame, and invokes an intrisic operation Value |
| RET | N/A | Pop the current stack frame and returns to caller |
