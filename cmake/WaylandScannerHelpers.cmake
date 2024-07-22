function(ws_generate type protocols input_file output_name)
    find_package(PkgConfig)
    pkg_get_variable(WAYLAND_PROTOCOLS ${protocols} pkgdatadir)
    pkg_get_variable(WAYLAND_SCANNER wayland-scanner wayland_scanner)

    if(NOT EXISTS ${input_file})
        set(input_file ${WAYLAND_PROTOCOLS}/${input_file})
    endif()

    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/protocols)

    execute_process(COMMAND ${WAYLAND_SCANNER}
        ${type}-header
        ${input_file}
        ${CMAKE_BINARY_DIR}/protocols/${output_name}.h
    )

    execute_process(COMMAND ${WAYLAND_SCANNER}
        public-code
        ${input_file}
        ${CMAKE_BINARY_DIR}/protocols/${output_name}.c
    )
endfunction()
