#pragma once

enum class Opcode
{
    PUSHA,
    PUSHB,
    POP,
    CALLA,
    CALLB,
    RET
};

enum class DataSource { ARGUMENT, SCOPE, DATA };
enum class DataDestination { ARGUMENT, SCOPE };

enum class Intrinsic { PRINT, PRINTLN };
