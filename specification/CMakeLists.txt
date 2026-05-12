if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Configure from the repository root: cmake -S . -B build")
endif()

# Documentation target (requires pandoc and xelatex)
# LaTeX packages needed (install via: sudo tlmgr install <pkg>):
#   framed, titling, collection-fontsrecommended
# Or install most common packages at once:
#   sudo tlmgr install collection-latexrecommended
find_program(PANDOC_EXECUTABLE pandoc)
if(PANDOC_EXECUTABLE)
    set(DOC_FILES
        "jz-hdl-specification"
        "simulation-specification"
        "testbench-specification"
        "chip-info-specification"
        "jzw-specification"
    )

    set(SPECIFICATION_PDF_OUTPUTS "")

    foreach(DOC_NAME ${DOC_FILES})
        set(SRC_MD "${CMAKE_CURRENT_SOURCE_DIR}/${DOC_NAME}.md")
        set(OUT_PDF "${CMAKE_CURRENT_BINARY_DIR}/${DOC_NAME}.pdf")

        add_custom_command(
            OUTPUT "${OUT_PDF}"
            COMMAND ${PANDOC_EXECUTABLE}
                "${SRC_MD}"
                -o "${OUT_PDF}"
                --pdf-engine=xelatex
                -V geometry:margin=1in
                -V fontsize=11pt
                --highlight-style=tango
                -V colorlinks=true
            DEPENDS "${SRC_MD}"
            COMMENT "Building PDF for ${DOC_NAME}"
        )

        list(APPEND SPECIFICATION_PDF_OUTPUTS "${OUT_PDF}")
    endforeach()

    add_custom_target(docs
        DEPENDS ${SPECIFICATION_PDF_OUTPUTS}
        COMMENT "Building specification PDFs in the build tree"
    )

    include(GNUInstallDirs)
    install(FILES ${SPECIFICATION_PDF_OUTPUTS}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/jz-hdl/specification
        COMPONENT Docs
        OPTIONAL
    )
else()
    message(STATUS "pandoc not found - 'docs' target will not be available")
endif()
