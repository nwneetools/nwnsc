add_library(nsclib
        Nsc.h
        NscCodeGenerator.cpp
        NscCodeGenerator.h
        NscCompat.h
        NscCompiler.cpp
        NscContext.cpp
        NscContext.h
        NscDecompiler.cpp
        NscIntrinsicDefs.h
        NscParser.cpp
        NscParser.hpp
        NscParserRoutines.cpp
        NscPCodeEnumerator.cpp
        NscPCodeEnumerator.h
        NscPStackEntry.cpp
        NscPStackEntry.h
        NscSymbolTable.h
        NwnDefines.cpp
        NwnDefines.h
        NwnDoubleLinkList.h
        NwnLoader.cpp
        NwnLoader.h
        NwnStreams.h
        Precomp.h
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
