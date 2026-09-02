cmake_minimum_required(VERSION 3.20)

project(aqueduct VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

if(WIN32)
    set(RCLONE_VENDOR_BIN "${CMAKE_SOURCE_DIR}/vendor/windows/librclone.dll")
    set(RCLONE_VENDOR_BIN_NAME "librclone.dll")
elseif(UNIX)
    set(RCLONE_VENDOR_BIN "${CMAKE_SOURCE_DIR}/vendor/linux/librclone.so")
    set(RCLONE_VENDOR_BIN_NAME "librclone.so")
else()
    message(FATAL_ERROR "librclone: unsupported platform")
endif()

if(NOT EXISTS "${RCLONE_VENDOR_BIN}")
    message(FATAL_ERROR "librclone: vendored binary not found at ${RCLONE_VENDOR_BIN}")
endif()

set(AQUEDUCT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/internal/librclone_shim.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/aqueduct.cpp"
)

if(BUILD_SHARED_LIBS)
    add_library(aqueduct SHARED ${AQUEDUCT_SOURCES})
else()
    add_library(aqueduct STATIC ${AQUEDUCT_SOURCES})
endif()

# Copy the vendored dynamic lib next to the built aqueduct artifact,
# so it's always a sibling file at both build time (for running tests /
# consumer dev builds straight from the build tree) and install time.
add_custom_command(
    TARGET aqueduct POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${RCLONE_VENDOR_BIN}"
            "$<TARGET_FILE_DIR:aqueduct>/${RCLONE_VENDOR_BIN_NAME}"
    COMMENT "Copying ${RCLONE_VENDOR_BIN_NAME} next to aqueduct"
    VERBATIM
)

# --- Include directories ---

target_include_directories(aqueduct PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(aqueduct PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

include(GNUInstallDirs)

install(TARGETS aqueduct
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES "${RCLONE_VENDOR_BIN}"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RENAME "${RCLONE_VENDOR_BIN_NAME}"
)
