#include "Assembler.hpp"
#include "Constructs.hpp"
#include "MemManip.hpp"

#include <magic_enum/magic_enum.hpp>
#include <argparse/argparse.hpp>
#include <libcoro/Generator.hpp>
#include <libcoro/Task.hpp>
#include <liberror/Result.hpp>
#include <liberror/Try.hpp>

#include <cassert>
#include <iostream>
#include <ranges>
#include <span>
#include <stack>
#include <variant>

using namespace liberror;
using namespace libcoro;

template <class ... T>
struct Visitor : T ... { using T::operator()...; };

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

struct Constant
{
    int data;
};

struct Relative
{
    DataSource source;
    int data;
};

using Value = std::variant<Constant, Relative>;

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

struct Frame
{
    int32_t returnAddress;

    std::vector<Value> argument;
    std::vector<Value> scope;
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
    Result<uint8_t> tick();

    Result<void> ret();
    Result<void> call();
    Result<void> call(Intrinsic intrinsic);
    Result<void> pop(DataDestination destination);
    Result<void> push(Value value, DataDestination destination);

private:
    int32_t programCounter_;
    std::stack<Frame> stack_;
    Memory memory_;

    Program program_;
};

Result<uint8_t> Machine::tick()
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

    programCounter_ = program_.entryPoint;

    return {};
}

Result<void> Machine::ret()
{
    programCounter_ = stack_.top().returnAddress;
    stack_.pop();
    return {};
}

Result<void> Machine::call()
{
    Frame frame {};
    frame.returnAddress = programCounter_;

    while (!stack_.top().argument.empty())
    {
        auto argument = stack_.top().argument.back();
        frame.scope.push_back(argument);
        stack_.top().argument.pop_back();
    }

    stack_.push(frame);

    programCounter_ = TRY(tick());

    return {};

}

Result<void> Machine::call(Intrinsic intrinsic)
{
    std::vector<Value> parameters {};

    while (!stack_.top().argument.empty())
    {
        auto argument = stack_.top().argument.back();
        parameters.push_back(argument);
        stack_.top().argument.pop_back();
    }

    switch (intrinsic)
    {
    case Intrinsic::PRINT: {
        [&] (this auto&& self, Value value) -> void {
            std::visit(Visitor {
                [&] (Constant value) { fmt::print("{}", value.data); },
                [&] (Relative value) {
                    switch (value.source)
                    {
                    case DataSource::SCOPE: {
                        self(stack_.top().scope.at(size_t(value.data)));
                        break;
                    }
                    case DataSource::DATA: {
                        auto offset = program_.address + program_.dataSegmentStart;
                        auto bytes = memory_.fetch(offset + 4, value.data, offset);
                        std::string_view data(reinterpret_cast<char*>(bytes.data()), size_t(offset));
                        fmt::print("{}", data);
                        break;
                    }
                    case DataSource::ARGUMENT: assert("UNREACHABLE" && false);
                    }
                }
            }, value);
        }(parameters.at(0));
        break;
    }
    case Intrinsic::PRINTLN: {
        [&] (this auto&& self, Value value) -> void {
            std::visit(Visitor {
                [&] (Constant value) { fmt::println("{}", value.data); },
                [&] (Relative value) {
                    switch (value.source)
                    {
                    case DataSource::SCOPE: {
                        self(stack_.top().scope.at(size_t(value.data)));
                        break;
                    }
                    case DataSource::DATA: {
                        auto offset = program_.address + program_.dataSegmentStart;
                        auto size = bytes_2_int(memory_.fetch(offset, value.data, 4));
                        auto bytes = memory_.fetch(offset + 4, value.data, size);
                        std::string_view data(reinterpret_cast<char*>(bytes.data()), size_t(size));
                        fmt::println("{}", data);
                        break;
                    }
                    case DataSource::ARGUMENT: assert("UNREACHABLE" && false);
                    }
                }
            }, value);
        }(parameters.at(0));
        break;
    }
    }

    return {};
}

Result<void> Machine::push(Value value, DataDestination destination)
{
    switch (destination)
    {
    case DataDestination::ARGUMENT: {
        stack_.top().argument.push_back(value);
        break;
    }
    case DataDestination::SCOPE: {
        stack_.top().scope.push_back(value);
        break;
    }
    }

    return {};
}

Result<void> Machine::pop(DataDestination destination)
{
    switch (destination)
    {
    case DataDestination::ARGUMENT: {
        stack_.top().argument.pop_back();
        break;
    }
    case DataDestination::SCOPE: {
        stack_.top().scope.pop_back();
        break;
    }
    }

    return {};
}

Result<void> Machine::execute()
{
    stack_.push({});

    while (!stack_.empty())
    {
        switch (Opcode(TRY(tick())))
        {
        case Opcode::PUSHA: {
            TRY(push(Constant(TRY(tick())), DataDestination(TRY(tick()))));
            break;
        }
        case Opcode::PUSHB: {
            TRY(push(Relative(DataSource(TRY(tick())), TRY(tick())), DataDestination(TRY(tick()))));
            break;
        }
        case Opcode::POP: {
            TRY(pop(DataDestination(TRY(tick()))));
            break;
        }
        case Opcode::CALLA: {
            TRY(call());
            break;
        }
        case Opcode::CALLB: {
            TRY(call(Intrinsic(TRY(tick()))));
            break;
        }
        case Opcode::RET: {
            TRY(ret());
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

    Machine machine;

    TRY(machine.load(TRY(assemble(source))));
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
