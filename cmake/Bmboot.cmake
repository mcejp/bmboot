function(Bmboot_PayloadPostBuild target)
    cmake_path(GET target STEM stem)

    if (NOT DEFINED CMAKE_SIZE)
        message(FATAL_ERROR "CMAKE_SIZE not defined")
    endif()

    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${target}>
            )

    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${target}> ${CMAKE_BINARY_DIR}/${stem}.bin
            COMMAND ${CMAKE_OBJDUMP} -dt $<TARGET_FILE:${target}> > ${CMAKE_BINARY_DIR}/${stem}.txt
            COMMENT "Building ${CMAKE_BINARY_DIR}/${CMAKE_BINARY_DIR}/${stem}.bin")
endfunction()
