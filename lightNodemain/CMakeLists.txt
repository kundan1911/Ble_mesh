set(srcs "main.c"
    "board.c")

idf_component_register(SRCS "nvsHeader.c" "${srcs}"
                    INCLUDE_DIRS  ".")
