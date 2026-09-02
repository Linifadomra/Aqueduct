cmake_minimum_required(VERSION 3.20)

project(aqueduct VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

set(AQUEDUCT_LIBRCLONE_REPO "Linifadomra/Aqueduct")
set(AQUEDUCT_LIBRCLONE_TAG  "librclone-v1.68.2")
set(AQUEDUCT_RELEASE_BASE
    "https://github.com/${AQUEDUCT_LIBRCLONE_REPO}/releases/download/${AQUEDUCT_LIBRCLONE_TAG}")
if(WIN32)
    set(RCLONE_VENDOR_BIN      "${CMAKE_SOURCE_DIR}/vendor/windows/librclone.dll")
    set(RCLONE_VENDOR_BIN_NAME "librclone.dll")
    set(RCLONE_VENDOR_URL      "${AQUEDUCT_RELEASE_BASE}/librclone.dll")
    set(RCLONE_VENDOR_SHA256   "5d9ebd6afac917fdfd250fc2e5a75cc2cb37eacd71c69cb20edd4fd0dddb608f")
elseif(UNIX)
    set(RCLONE_VENDOR_BIN      "${CMAKE_SOURCE_DIR}/vendor/linux/librclone.so")
    set(RCLONE_VENDOR_BIN_NAME "librclone.so")
    set(RCLONE_VENDOR_URL      "${AQUEDUCT_RELEASE_BASE}/librclone.so")
    set(RCLONE_VENDOR_SHA256   "ac888d87741c41938e58112fea6c068dba801c426613d0a654bd0bba03f5d362")
else()
    message(FATAL_ERROR "librclone: unsupported platform")
endif()

if(NOT EXISTS "${RCLONE_VENDOR_BIN}")
    message(STATUS "librclone: downloading ${RCLONE_VENDOR_BIN_NAME} from ${RCLONE_VENDOR_URL}")
    file(DOWNLOAD "${RCLONE_VENDOR_URL}" "${RCLONE_VENDOR_BIN}"
        EXPECTED_HASH SHA256=${RCLONE_VENDOR_SHA256}
        SHOW_PROGRESS
        STATUS RCLONE_DOWNLOAD_STATUS)
    list(GET RCLONE_DOWNLOAD_STATUS 0 RCLONE_DOWNLOAD_CODE)
    if(NOT RCLONE_DOWNLOAD_CODE EQUAL 0)
        list(GET RCLONE_DOWNLOAD_STATUS 1 RCLONE_DOWNLOAD_MSG)
        file(REMOVE "${RCLONE_VENDOR_BIN}") # don't leave a partial/corrupt file behind
        message(FATAL_ERROR "librclone: download failed: ${RCLONE_DOWNLOAD_MSG}")
    endif()
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
