set(srcs "open62541.c")
if(CONFIG_OPEN62541_ALOCAR_PSRAM)
    list(APPEND srcs "port/ua_arch_esp.c")
endif()

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS "include" "port/include"
                       PRIV_INCLUDE_DIRS "port/freertos_compat"
                       REQUIRES lwip
                       PRIV_REQUIRES heap)

# Inclui as definições de plataforma (ua_arch_esp.h) antes dos padrões do open62541.
# PUBLIC: todo código que inclui open62541.h precisa ver as mesmas definições.
target_compile_definitions(${COMPONENT_LIB} PUBLIC "UA_ARCH_HEADER=\"ua_arch_esp.h\"")

# Código de terceiros: avisos não devem interromper o build
set_source_files_properties(open62541.c PROPERTIES COMPILE_OPTIONS "-Wno-error;-w")
