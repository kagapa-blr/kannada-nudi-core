cmake_minimum_required(VERSION 3.16)

project(kannada-nudi VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(NUDI_CORE_DIR "${CMAKE_SOURCE_DIR}/src/core")
set(NUDI_PLATFORM_DIR "${CMAKE_SOURCE_DIR}/src/platform")

# ============================================================
# Shared core library
# ============================================================

add_library(nudi-composer STATIC
    "${NUDI_CORE_DIR}/nudi_composer.cpp"
)

target_include_directories(
    nudi-composer
    PUBLIC
        "${NUDI_CORE_DIR}"
)

# ============================================================
# Linux-only targets
# ============================================================

if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(IBUS REQUIRED IMPORTED_TARGET ibus-1.0)
    pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)

    add_executable(ibus-engine-nudi
        "${NUDI_PLATFORM_DIR}/linux/ibus_engine.cpp"
    )

    target_link_libraries(ibus-engine-nudi
        PRIVATE
            nudi-composer
            PkgConfig::IBUS
    )

    add_executable(nudi-editor
        "${NUDI_PLATFORM_DIR}/linux/nudi_editor.cpp"
    )

    target_link_libraries(
        nudi-editor
        PRIVATE
            nudi-composer
            PkgConfig::GTK3
    )

    include(GNUInstallDirs)

    install(
        TARGETS ibus-engine-nudi
        RUNTIME DESTINATION libexec
    )

    install(
        TARGETS nudi-editor
        RUNTIME DESTINATION bin
    )

    install(
        FILES "${CMAKE_SOURCE_DIR}/com.kagapa.Nudi.xml"
        DESTINATION share/ibus/component
    )

    install(
        FILES "${CMAKE_SOURCE_DIR}/resources/icons/nudi.png"
        DESTINATION share/icons/hicolor/48x48/apps
        RENAME nudi.png
    )

    install(
        FILES "${CMAKE_SOURCE_DIR}/resources/icons/nudi.png"
        DESTINATION share/icons/hicolor/64x64/apps
        RENAME nudi.png
    )

    install(
        FILES "${CMAKE_SOURCE_DIR}/resources/icons/nudi.png"
        DESTINATION share/icons/hicolor/128x128/apps
        RENAME nudi.png
    )

    install(
        FILES "${CMAKE_SOURCE_DIR}/resources/nudi.desktop"
        DESTINATION share/applications
    )
endif()

# ============================================================
# Windows target
# ============================================================

if(WIN32)
    add_executable(nudi-windows
        "${NUDI_PLATFORM_DIR}/windows/nudi_windows.cpp"
    )

    target_include_directories(
        nudi-windows
        PRIVATE
            "${NUDI_CORE_DIR}"
    )

    target_link_libraries(nudi-windows
        PRIVATE
            nudi-composer
    )

    include(GNUInstallDirs)

    install(
        TARGETS nudi-windows
        RUNTIME DESTINATION bin
    )
endif()

# ============================================================
# Tests
# ============================================================

enable_testing()

add_executable(
    test-composer
    tests/nudi_composer_test.cpp
    tests/nudi_test_cases.cpp
)

target_include_directories(
    test-composer
    PRIVATE
        tests
        "${NUDI_CORE_DIR}"
)

set(
    NUDI_TEST_OUTPUT_DIR
    "${CMAKE_SOURCE_DIR}/output"
)

file(MAKE_DIRECTORY "${NUDI_TEST_OUTPUT_DIR}")

target_compile_definitions(
    test-composer
    PRIVATE
        NUDI_TEST_OUTPUT_DIR="${NUDI_TEST_OUTPUT_DIR}"
)

target_link_libraries(
    test-composer
    PRIVATE
        nudi-composer
)

add_test(
    NAME composer
    COMMAND test-composer
)

if(UNIX AND NOT APPLE)
    add_test(
        NAME ibus-registration
        COMMAND sh ${CMAKE_SOURCE_DIR}/tests/check_ibus_registration.sh
    )
endif()
