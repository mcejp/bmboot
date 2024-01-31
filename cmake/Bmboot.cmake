set(BMBOOT_ALL_CPUS 1 2 3)


# see build.rst for usage information
function(add_bmboot_payload NAME)
    set(ALL_TARGETS)

    # Use an "object library" so that source files will be compiled only once
    add_library("${NAME}" OBJECT ${ARGN})

    target_link_libraries("${NAME}" PRIVATE bmboot_payload_runtime)

    # link the payload separately for each CPU core
    foreach(CPU ${BMBOOT_ALL_CPUS})
        set(TARGET "${NAME}_cpu${CPU}")
        add_executable("${TARGET}" $<TARGET_OBJECTS:${NAME}>)
        target_link_libraries("${TARGET}" PRIVATE "${NAME}")
        set_target_properties("${TARGET}" PROPERTIES SUFFIX ".elf")

        target_link_options(${TARGET} PUBLIC
                -specs=nosys.specs
                -Wl,-T,${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/executor/payload/payload_cpu${CPU}.ld)

        Bmboot_PayloadPostBuild("${TARGET}")

        list(APPEND ALL_TARGETS "${TARGET}")
    endforeach()

    set("${NAME}_TARGETS" "${ALL_TARGETS}" PARENT_SCOPE)
endfunction()


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
