# kubo

Simple virtual machine i'm making for fun.

<img width="15%" alt="image" src="https://github.com/user-attachments/assets/ae84fc45-b053-4a26-ba7d-d54f92afb0ec" />

# Building

You will need a [C++23 compiler](https://github.com/llvm/llvm-project/releases) and [cmake](https://cmake.org/) installed.

```bash
python configure.py && python build.py
```

# Example

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
