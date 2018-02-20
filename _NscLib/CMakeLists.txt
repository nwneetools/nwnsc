add_library(nsclib
    Nsc
        NscCodeGenerator
        NscCompat
        NscCompiler
        NscContext
        NscDecompiler
        NscIntrinsicDefs
        NscParser
        NscParserRoutines
        NscPCodeEnumerator
        NscPStackEntry
        NscPStackEntry
        NscSymbolTable
        NwnArray
        NwnDefines
        NwnDoubleLinkList
        NwnLoader
        NwnStreams
        Precomp
)

    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/NscParser.cpp ${CMAKE_CURRENT_SOURCE_DIR}/NscParser.hpp
            DEPENDS NscParser.ypp
            COMMAND ${BISON_EXECUTABLE} -S lalr1.cc ${CMAKE_CURRENT_SOURCE_DIR}/NscParser.ypp -o ${CMAKE_CURRENT_SOURCE_DIR}/NscParser.cpp
    )
