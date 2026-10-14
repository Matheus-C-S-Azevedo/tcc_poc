idf_component_register(SRCS "open62541.c"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS "port"
                       REQUIRES lwip)

# Código de terceiros: avisos não devem interromper o build
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error -w)
