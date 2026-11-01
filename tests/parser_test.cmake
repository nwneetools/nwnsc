# Run each case in isolation: compilation writes its output into the working directory.
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
file(COPY "${FIXTURES}/" DESTINATION "${WORK}")
execute_process(
    COMMAND "${COMPILER}" -i "${WORK}" "${CASE}.nss"
    WORKING_DIRECTORY "${WORK}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
    TIMEOUT 5)
set(output "${stdout}${stderr}")

if(NOT "${result}" MATCHES "^-?[0-9]+$")
    message(FATAL_ERROR "Compiler crashed or timed out: ${result}\n${output}")
endif()

if(CASE STREQUAL "valid" OR CASE STREQUAL "mixed_case_include")
    if(NOT result EQUAL 0 OR NOT EXISTS "${WORK}/${CASE}.ncs")
        message(FATAL_ERROR "Valid script did not produce bytecode (${result}):\n${output}")
    endif()
else()
    if(result EQUAL 0 OR EXISTS "${WORK}/${CASE}.ncs")
        message(FATAL_ERROR "Invalid script was accepted:\n${output}")
    endif()
endif()

string(REGEX MATCHALL "Error: NSC1040:" syntax_matches "${output}")
list(LENGTH syntax_matches actual_syntax_errors)
if(NOT actual_syntax_errors EQUAL SYNTAX_ERRORS)
    message(FATAL_ERROR "Expected ${SYNTAX_ERRORS} syntax errors, got ${actual_syntax_errors}:\n${output}")
endif()

set(expected_errors ${SYNTAX_ERRORS})
if(CASE STREQUAL "unknown_symbol")
    set(expected_errors 1)
    if(NOT output MATCHES "Error: NSC1020:")
        message(FATAL_ERROR "Missing undeclared-identifier diagnostic:\n${output}")
    endif()
elseif(CASE STREQUAL "many_errors")
    set(expected_errors 33)
    if(NOT output MATCHES "Error: NSC1041:")
        message(FATAL_ERROR "Missing error-limit diagnostic:\n${output}")
    endif()
endif()
string(REGEX MATCHALL "Error: NSC[0-9]+:" all_errors "${output}")
list(LENGTH all_errors actual_errors)
if(NOT actual_errors EQUAL expected_errors)
    message(FATAL_ERROR "Unexpected extra or missing diagnostics:\n${output}")
endif()
