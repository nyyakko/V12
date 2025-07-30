#include <argparse/argparse.hpp>
#include <libcoro/Generator.hpp>
#include <libcoro/Task.hpp>
#include <libenum/Enum.hpp>
#include <liberror/Result.hpp>
#include <liberror/Try.hpp>

#include <cassert>
#include <fstream>
#include <iostream>
#include <span>
#include <stack>

using namespace liberror;
using namespace libcoro;

ENUM_CLASS(Instruction,
    PUSH,
    PUSHI,
    POP,
    CALL,
    CALLI,
    RET
);

ENUM_CLASS(Intrinsic, PRINTLN);

ENUM_CLASS(Stack, FRAME, ARGUMENT, SCOPE)

class Machine
{
public:
    explicit Machine(std::vector<uint8_t> const& program)
        : program_(program)
        , programCounter_()
        , frame_()
        , argument_()
        , scope_()
    {}

public:
    Result<void> execute();

private:
    Result<void> ret();
    Result<void> call();
    Result<void> calli(Intrinsic intrinsic);
    Result<void> pop();
    Result<void> push(int value, Stack destination);
    Result<void> pushi(Stack source, int index, Stack destination);

    Result<int> fetch(Stack source, int index = -1)
    {
        switch (source)
        {
        case Stack::FRAME: assert("UNIMPLEMENTED" && false);
        case Stack::ARGUMENT: {
            auto value = argument_.top();
            argument_.pop();
            return value;
        }
        case Stack::SCOPE: return scope_.at(static_cast<size_t>(index));
        }

        return make_error("Tried to fetch from unknown location {}", source.to_int());
    }

    Result<uint8_t> advance()
    {
        if (programCounter_ >= program_.size())
        {
            return make_error("Tried to advance past the program size");
        }

        return program_.at(programCounter_++);
    }

private:
    std::vector<uint8_t> program_;
    size_t programCounter_;

    std::vector<int> frame_;
    std::stack<int> argument_;
    std::vector<int> scope_;
};

Result<void> Machine::calli(Intrinsic intrinsic)
{
    switch (intrinsic)
    {
    case Intrinsic::PRINTLN: {
        fmt::println("{}", TRY(fetch(Stack::ARGUMENT)));
        return {};
    }
    }

    return make_error("Tried to call unknown intrinsic {}", intrinsic.to_int());
}

Result<void> Machine::push(int value, Stack destination)
{
    switch (destination)
    {
    case Stack::FRAME: {
        assert("UNIMPLEMENTED" && false);
        return {};
    }
    case Stack::ARGUMENT: {
        argument_.push(value);
        return {};
    }
    case Stack::SCOPE: {
        scope_.push_back(value);
        return {};
    }
    }

    return make_error("Tried to push to unknown location {}", destination.to_int());
}

Result<void> Machine::pushi(Stack source, int index, Stack destination)
{
    return push(TRY(fetch(source, index)), destination);
}

Result<void> Machine::execute()
{
    while (programCounter_ < program_.size())
    {
        switch (TRY(advance()))
        {
        case Instruction::PUSH: {
            TRY(push(TRY(advance()), Stack::from_int(TRY(advance()))));
            break;
        }
        case Instruction::PUSHI: {
            TRY(pushi(Stack::from_int(TRY(advance())), TRY(advance()), Stack::from_int(TRY(advance()))));
            break;
        }
        case Instruction::POP: {
            assert("UNIMPLEMENTED" && false);
            break;
        }
        case Instruction::CALL: {
            assert("UNIMPLEMENTED" && false);
            break;
        }
        case Instruction::CALLI: {
            TRY(calli(Intrinsic::from_int(TRY(advance()))));
            break;
        }
        case Instruction::RET: {
            assert("UNIMPLEMENTED" && false);
            break;
        }
        }
    }

    return {};
}

Result<void> safe_main(std::span<char const*> arguments)
{
    argparse::ArgumentParser args("V12", "", argparse::default_arguments::help);
    args.add_description("V12 virtual machine");

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

    std::ifstream stream(source);

    std::vector<uint8_t> program;

    for (uint8_t instruction; stream >> instruction;)
    {
        program.push_back(instruction);
    }

    Machine machine(program);
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
