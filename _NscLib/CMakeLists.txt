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
        NwnDefines
        NwnDoubleLinkList
        NwnLoader
        NwnStreams
        Precomp
)

if (WIN32 OR WIN64)
    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/NscParser.cpp ${CMAKE_CURRENT_SOURCE_DIR}/NscParser.hpp
            DEPENDS NscParser.ypp
            COMMAND "${BISON_EXECUTABLE}" -S lalr1.cc ${CMAKE_CURRENT_SOURCE_DIR}/NscParser.ypp -o ${CMAKE_CURRENT_SOURCE_DIR}/NscParser.cpp
    )
else ()
    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/NscParser.cpp ${CMAKE_CURRENT_SOURCE_DIR}/NscParser.hpp
            DEPENDS NscParser.ypp
            COMMAND "${BISON_EXECUTABLE}" -S lalr1.cc ${CMAKE_CURRENT_SOURCE_DIR}/NscParser.ypp -o ${CMAKE_CURRENT_SOURCE_DIR}/NscParser.cpp
			)
endif (WIN32 OR WIN64)
