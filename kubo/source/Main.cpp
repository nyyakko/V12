#include <cstdint>
#include <fmt/core.h>
#include <magic_enum/magic_enum.hpp>
#include <argparse/argparse.hpp>
#include <libcoro/Generator.hpp>
#include <libcoro/Task.hpp>
#include <liberror/Result.hpp>
#include <liberror/Try.hpp>

#include <cassert>
#include <fstream>
#include <iostream>
#include <ranges>
#include <span>
#include <stack>
#include <variant>

using namespace liberror;
using namespace libcoro;

template <class ... T>
struct Visitor : T ... { using T::operator()...; };

enum class Opcode
{
    CALL,
    LOAD,
    POP,
    PUSH,
    RET,
    STORE
};

enum class CallMode { EXTRINSIC, INTRINSIC };

enum class Intrinsic { PRINT, PRINTLN };

enum class DataSource { DATA_SEGMENT, LOCAL_SCOPE, GLOBAL_SCOPE };
enum class DataDestination { LOCAL_SCOPE, GLOBAL_SCOPE };

inline int32_t bytes_2_int(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    return static_cast<int>(a << 24 | b << 16 | c << 8  | d << 0);
}

inline int32_t bytes_2_int(std::span<uint8_t> bytes)
{
    assert(bytes.size() == 4);
    return bytes_2_int(bytes[0], bytes[1], bytes[2], bytes[3]);
}

inline int32_t bytes_2_int(uint8_t* bytes)
{
    assert(bytes);
    return bytes_2_int(bytes[0], bytes[1], bytes[2], bytes[3]);
}

inline std::array<uint8_t, 4> int_2_bytes(int value)
{
    return {
        static_cast<uint8_t>((value >> 24) & 0xFF),
        static_cast<uint8_t>((value >> 16) & 0xFF),
        static_cast<uint8_t>((value >> 8)  & 0xFF),
        static_cast<uint8_t>((value >> 0)  & 0xFF),
    };
}

static constexpr std::string_view MAGIC = "This is a kubo program";

struct [[gnu::packed]] Program
{
    char magic[MAGIC.size()] {};

    int32_t address;
    int32_t dataSegmentStart;
    int32_t codeSegmentStart;
    int32_t entryPoint;
    int32_t size;
};

using Value = std::variant<bool, char, short, float, int, uint8_t*>;

class Memory
{
public:
    explicit Memory(size_t sizeInBytes)
    {
        memory_.reserve(sizeInBytes);
    }

public:
    // cppcheck-suppress [functionStatic, constParameterReference]
    auto& get(this auto& self) { return self.memory_; }

    void push(uint8_t data);
    std::span<uint8_t> fetch(int32_t offset, int32_t index, int32_t sizeInBytes);
    uint8_t fetch(int32_t offset, int32_t index);
    uint8_t* reference(int32_t offset, int32_t index);

private:
    std::vector<uint8_t> memory_;
};

void Memory::push(uint8_t data)
{
    memory_.push_back(data);
}

std::span<uint8_t> Memory::fetch(int32_t offset, int32_t index, int32_t sizeInBytes)
{
    return { memory_.data() + offset + index, size_t(sizeInBytes) };
}

uint8_t Memory::fetch(int32_t offset, int32_t index)
{
    return fetch(offset, index, 1)[0];
}

uint8_t* Memory::reference(int32_t offset, int32_t index)
{
    return memory_.data() + offset + index;
}

struct Frame
{
    int32_t returnAddress;

    std::stack<Value> operands;
    std::vector<Value> local;
};

class Machine
{
public:
    explicit Machine()
        : programCounter_()
        , stack_()
        , memory_(8 * 1024 * 1024)
        , program_()
    {}

public:
    Result<void> load(std::vector<uint8_t> const& program);
    Result<void> execute();

private:
    Result<uint8_t> fetch();

    Result<void> call(CallMode mode);
    Result<void> load(DataSource source);
    Result<void> pop();
    Result<void> push();
    Result<void> ret();
    Result<void> store(DataDestination destination);

private:
    int32_t programCounter_;
    std::stack<Frame> stack_;
    Memory memory_;

    Program program_;
};

Result<uint8_t> Machine::fetch()
{
    if (program_.address + programCounter_ > program_.address + program_.size)
    {
        return make_error("Tried to advance past program size");
    }

    return memory_.fetch(program_.address, programCounter_++);
}

Result<void> Machine::load(std::vector<uint8_t> const& program)
{
    if (std::string_view(reinterpret_cast<char const*>(program.data()), sizeof(Program::magic)) != MAGIC)
    {
        return make_error("Not a valid kubo program");
    }

    static constexpr auto magic = sizeof(Program::magic);

    program_ = {
        .address = static_cast<int32_t>(memory_.get().size()),
        .dataSegmentStart = bytes_2_int(program.at(magic + 0), program.at(magic + 1), program.at(magic + 2),  program.at(magic + 3)),
        .codeSegmentStart = bytes_2_int(program.at(magic + 4), program.at(magic + 5), program.at(magic + 6),  program.at(magic + 7)),
        .entryPoint       = bytes_2_int(program.at(magic + 8), program.at(magic + 9), program.at(magic + 10), program.at(magic + 11)),
        .size = static_cast<int32_t>(program.size() - (sizeof(Program) - 8))
    };

    for (auto const& byte : program | std::views::drop(sizeof(Program) - 8))
    {
        memory_.push(byte);
    }

    programCounter_ = program_.codeSegmentStart + program_.entryPoint;

    return {};
}

Result<void> Machine::call(CallMode mode)
{
    switch (mode)
    {
    case CallMode::EXTRINSIC: {
        Frame frame {};
        frame.returnAddress = programCounter_ + 1;

        while (!(stack_.empty() || stack_.top().operands.empty()))
        {
            auto operand = stack_.top().operands.top();
            frame.local.push_back(operand);
            stack_.top().operands.pop();
        }

        stack_.push(std::move(frame));

        programCounter_ = program_.codeSegmentStart + TRY(fetch());

        break;
    }
    case CallMode::INTRINSIC: {
        using sv = std::string_view;

        switch (Intrinsic(TRY(fetch())))
        {
        case Intrinsic::PRINT: {
            std::visit(Visitor {
                [&] (auto value) { fmt::print("{}", value); },
                [&] (uint8_t* reference) {
                    fmt::print("{}", sv(reinterpret_cast<char*>(std::next(reference, 4)), size_t(bytes_2_int(reference))));
                }
            }, stack_.top().operands.top());
            break;
        }
        case Intrinsic::PRINTLN: {
            std::visit(Visitor {
                [&] (auto value) { fmt::println("{}", value); },
                [&] (uint8_t* reference) {
                    fmt::println("{}", sv(reinterpret_cast<char*>(std::next(reference, 4)), size_t(bytes_2_int(reference))));
                }
            }, stack_.top().operands.top());
            break;
        }
        }
        break;
    }
    }

    return {};
}

Result<void> Machine::load(DataSource source)
{
    auto offset = bytes_2_int(TRY(fetch()), TRY(fetch()), TRY(fetch()), TRY(fetch()));

    switch (source)
    {
    case DataSource::DATA_SEGMENT: {
        stack_.top().operands.push(memory_.reference(program_.address + program_.dataSegmentStart, offset));
        break;
    }
    case DataSource::LOCAL_SCOPE: {
        stack_.top().operands.push(stack_.top().local.at(size_t(offset)));
        break;
    }
    case DataSource::GLOBAL_SCOPE: {
        assert("UNIMPLEMENTED" && false);
        break;
    }
    }

    return {};
}

Result<void> Machine::pop()
{
    if (stack_.top().operands.empty())
    {
        return make_error("Operand stack was empty");
    }

    stack_.top().operands.pop();

    return {};
}

Result<void> Machine::push()
{
    stack_.top().operands.push(bytes_2_int(TRY(fetch()), TRY(fetch()), TRY(fetch()), TRY(fetch())));
    return {};
}

Result<void> Machine::ret()
{
    auto result = std::move(stack_.top().operands.top());
    programCounter_ = stack_.top().returnAddress;

    stack_.pop();

    if (!stack_.empty())
    {
        stack_.top().operands.push(std::move(result));
    }

    return {};
}

Result<void> Machine::store(DataDestination destination)
{
    auto offset = bytes_2_int(TRY(fetch()), TRY(fetch()), TRY(fetch()), TRY(fetch()));

    if (stack_.top().operands.empty())
    {
        return make_error("Operand stack was empty");
    }

    auto value = stack_.top().operands.top();
    stack_.top().operands.pop();

    switch (destination)
    {
    case DataDestination::LOCAL_SCOPE: {
        if (stack_.top().local.empty())
            stack_.top().local.emplace_back(std::move(value));
        else
            stack_.top().local.at(size_t(offset)) = std::move(value);
        break;
    }
    case DataDestination::GLOBAL_SCOPE: {
        assert("UNIMPLEMENTED" && false);
        break;
    }
    }

    return {};
}

Result<void> Machine::execute()
{
    /*
     * 00000'000
     * ┬──── ┬──
     * |     ╰───▶ Instruction Mode
     * ╰─────────▶ Instruction
     */

    Frame entry {};
    stack_.push(entry);

    while (!stack_.empty())
    {
        auto instruction = TRY(fetch());

        switch (auto mode = (instruction >> 0) & 7; Opcode((instruction >> 3) & 31))
        {
        case Opcode::CALL: {
            TRY(call(CallMode(mode)));
            break;
        }
        case Opcode::LOAD: {
            TRY(load(DataSource(TRY(fetch()))));
            break;
        }
        case Opcode::POP: {
            TRY(pop());
            break;
        }
        case Opcode::PUSH: {
            TRY(push());
            break;
        }
        case Opcode::RET: {
            TRY(ret());
            break;
        }
        case Opcode::STORE: {
            TRY(store(DataDestination(TRY(fetch()))));
            break;
        }
        }
    }

    return {};
}

Result<void> safe_main(std::span<char const*> arguments)
{
    argparse::ArgumentParser args("kubo", "", argparse::default_arguments::help);
    args.add_description("kubo virtual machine");

    args.add_argument("-f", "--file").help("bytecode to be executed").required();

    try
    {
        args.parse_args(static_cast<int>(arguments.size()), arguments.data());
    }
    catch (std::exception const& exception)
    {
        return liberror::make_error(exception.what());
    }

    auto source = args.get<std::string>("--file");

    if (!std::filesystem::exists(source))
    {
        return make_error("source {} does not exist.", source);
    }

    std::ifstream stream(source, std::ios::binary);

    stream.seekg(0, std::ios::end);
    std::vector<uint8_t> program(static_cast<size_t>(stream.tellg()));
    stream.seekg(0, std::ios::beg);
    stream.read(reinterpret_cast<char*>(program.data()), static_cast<int>(program.size()));

    Machine machine;

    TRY(machine.load(program));
    TRY(machine.execute());

    return {};
}

int main(int argc, char const** argv)
{
    auto result = safe_main(std::span<char const*>(argv, size_t(argc)));

    if (!result.has_value())
    {
        std::cout << result.error().message() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
