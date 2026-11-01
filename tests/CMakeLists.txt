function(parser_test name syntax_errors)
    add_test(NAME parser_${name}
        COMMAND ${CMAKE_COMMAND}
            "-DCOMPILER=$<TARGET_FILE:nwnsc>"
            "-DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
            "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/${name}"
            "-DCASE=${name}"
            "-DSYNTAX_ERRORS=${syntax_errors}"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/parser_test.cmake)
    set_tests_properties(parser_${name} PROPERTIES TIMEOUT 15)
endfunction()

parser_test(valid 0)
parser_test(mixed_case_include 0)
parser_test(missing_semicolon 1)
parser_test(missing_brace 1)
parser_test(bad_operator 1)
parser_test(bad_declaration 1)
parser_test(bad_top_level 1)
parser_test(unknown_symbol 0)
parser_test(multiple_errors 2)
parser_test(function_boundary 1)
parser_test(nested_boundary 1)
parser_test(function_second_error 2)
parser_test(ampersand_recovery 2)
parser_test(many_errors 32)
