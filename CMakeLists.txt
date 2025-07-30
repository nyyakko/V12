cmake_minimum_required(VERSION 3.25)

project(V12 LANGUAGES CXX)

include(cmake/get_cpm.cmake)

CPMAddPackage(URI "gh:nyyakko/LibEnum#master"  EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:nyyakko/LibError#master" EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:nyyakko/LibCoro#master"  EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:fmtlib/fmt#10.2.1"       EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:nyyakko/argparse#master" EXCLUDE_FROM_ALL YES)

include(cmake/static_analyzers.cmake)

set(V12_CompilerOptions ${V12_CompilerOptions} -Wno-gnu-statement-expression-from-macro-expansion)
# set(V12_LinkerOptions ${V12_LinkerOptions})

set(V12_ExternalLibraries
    LibError::LibError
    LibEnum::LibEnum
    LibCoro::LibCoro
    argparse::argparse
    fmt::fmt
)

add_subdirectory(V12)
