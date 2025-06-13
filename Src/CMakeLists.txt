target_sources(
    ${CMAKE_PROJECT_NAME}
    PUBLIC
    main.c
    syscall.c
    sysmem.c
    system_stm32h7xx.c

    cmds.c
    cmds.h

    etherlib_options.c
)

target_include_directories(
    ${CMAKE_PROJECT_NAME}
    PUBLIC
    .
)

add_subdirectory(standard_output)
add_subdirectory(cliutils)
add_subdirectory(ethernet)