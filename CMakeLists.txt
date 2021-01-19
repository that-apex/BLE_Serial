cmake_minimum_required(VERSION 3.16)
project(BLE_Serial CXX)

option(BLE_SERIAL_BUILD_EXECUTABLE    "Should the executable be built?"    ON)
option(BLE_SERIAL_BUILD_DOCUMENTATION "Should the documentation be built?" OFF)

set(CMAKE_CXX_STANDARD 20)

if (WIN32)
    set(PLATFORM_SOURCES
            src/platform/windows/bluetooth.cpp
            src/platform/windows/com.cpp
    )

    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /await")
    endif()
endif()

# Library
add_library(BLE_Serial_Lib STATIC
        src/bluetooth.cpp
        src/com.cpp
        ${PLATFORM_SOURCES}
)

target_include_directories(BLE_Serial_Lib
        PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# Executable
if (BLE_SERIAL_BUILD_EXECUTABLE)
    add_executable(BLE_Serial
            src/main.cpp
    )

    target_link_libraries(BLE_Serial
            PUBLIC
                BLE_Serial_Lib
    )
endif()

# Documentation
if (BLE_SERIAL_BUILD_DOCUMENTATION)
    find_package(Doxygen)

    if (DOXYGEN_FOUND)
        message(STATUS "Doxygen v${DOXYGEN_VERSION} found!")

        set(DOXYFILE "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")
        configure_file(
                "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in"
                "${DOXYFILE}"
                @ONLY
        )

        add_custom_target(BLE_Serial_Docs ALL
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE}
                COMMENT "Generating API documentation with Doxygen"
                VERBATIM
        )
    else()
        message(FATAL_ERROR "BLE_SERIAL_BUILD_DOCUMENTATION was specified but Doxygen couldn't be find")
    endif()
endif()
