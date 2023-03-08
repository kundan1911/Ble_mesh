set(srcs "main.c"
    "board.c"
    "uart_handler.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS  ".")
