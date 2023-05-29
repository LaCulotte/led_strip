set(SOURCES
  octopus-utils/octopus-app/app.cpp
  octopus-utils/octopus-connection/connection.cpp
)

set(HEADERS
  octopus-connection/json.hpp

  octopus-app/work.hpp
  octopus-app/app.hpp
  octopus-connection/connection.hpp
)


idf_component_register(SRCS "wifi.c" "esp_connection.cpp" "esp_app.cpp" "main.c" "octopus_link.cpp" ${SOURCES}
                    INCLUDE_DIRS "." "./octopus-utils")

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=parentheses)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=dangling-else)
target_compile_options(${COMPONENT_LIB} PRIVATE -fexceptions)