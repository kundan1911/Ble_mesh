set(srcs "main.c"
    "board.c" 
    "nvs_header.c"
    "uart_handler.c")

idf_component_register(SRCS "uart_handler.c" "nvs_header.c" "${srcs}"
                    INCLUDE_DIRS  ".")
