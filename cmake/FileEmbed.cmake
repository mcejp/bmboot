# Adapted from original:
# https://jonathanhamberg.com/post/cmake-file-embedding/
# https://gitlab.com/jhamberg/cmake-examples/-/blob/master/cmake/FileEmbed.cmake

function(FileEmbed_Add input output c_name)
    add_custom_command(
            OUTPUT ${output}
            COMMAND ${CMAKE_COMMAND}
            -DRUN_FILE_EMBED_GENERATE=1
            -DINPUT_FILE=${input}
            -DOUTPUT_FILE=${output}
            -DC_NAME=${c_name}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FileEmbed.cmake
            MAIN_DEPENDENCY ${input}
    )
endfunction()

function(FileEmbed_Generate file output_filename c_name)
    file(READ ${file} content HEX)

    # Separate into individual bytes.
    string(REGEX MATCHALL "([A-Fa-f0-9][A-Fa-f0-9])" SEPARATED_HEX ${content})

    set(output_c "")

    set(counter 0)
    foreach (hex IN LISTS SEPARATED_HEX)
        string(APPEND output_c "0x${hex}, ")
        MATH(EXPR counter "${counter}+1")
        if (counter GREATER 16)
            string(APPEND output_c "\n    ")
            set(counter 0)
        endif ()
    endforeach ()

    set(output_c "#pragma once

#include <array>
#include <stdint.h>

static constexpr auto ${c_name} = std::to_array<uint8_t>({
    ${output_c}
})\;

")

    file(WRITE ${output_filename} ${output_c})
endfunction()

if (RUN_FILE_EMBED_GENERATE)
    FileEmbed_Generate(${INPUT_FILE} ${OUTPUT_FILE} ${C_NAME})
endif ()
