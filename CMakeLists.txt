cmake_minimum_required(VERSION 3.25)

project(kubo LANGUAGES CXX)

include(cmake/get_cpm.cmake)

CPMAddPackage(URI "gh:Neargye/magic_enum@0.9.7" EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:nyyakko/LibError#master"  EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:nyyakko/LibCoro#master"   EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:fmtlib/fmt#10.2.1"        EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:nyyakko/argparse#master"  EXCLUDE_FROM_ALL YES)

include(cmake/static_analyzers.cmake)

set(kubo_CompilerOptions ${kubo_CompilerOptions} -Wno-gnu-statement-expression-from-macro-expansion)
# set(kubo_LinkerOptions ${kubo_LinkerOptions})

set(kubo_ExternalLibraries
    LibError::LibError
    magic_enum::magic_enum
    LibCoro::LibCoro
    argparse::argparse
    fmt::fmt
)

add_subdirectory(kubo)
