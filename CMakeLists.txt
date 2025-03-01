cmake_minimum_required(VERSION 3.10)
project(MicroOS C ASM)

# Detect OS and architecture
if(WIN32)
    set(SDL2_DIR "${CMAKE_SOURCE_DIR}/external/SDL2" CACHE PATH "SDL2 directory")
    set(SDL2_TTF_DIR "${CMAKE_SOURCE_DIR}/external/SDL2_ttf" CACHE PATH "SDL2_ttf directory")
elseif(APPLE)
    set(SDL2_DIR "/opt/homebrew/Cellar/sdl2/2.32.0/include/SDL2" CACHE PATH "SDL2 directory")
    set(SDL2_TTF_DIR "/opt/homebrew/Cellar/sdl2_ttf/2.24.0/include/SDL2" CACHE PATH "SDL2_ttf directory")
elseif(UNIX)
    set(SDL2_DIR "/usr" CACHE PATH "SDL2 directory")
    set(SDL2_TTF_DIR "/usr" CACHE PATH "SDL2_ttf directory")
endif()

# Find required packages
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

# Include directories
include_directories(/opt/homebrew/include/SDL2)

# Source files
set(SOURCES
    microos.c
)

# Determine whether to use assembly or C implementation
set(USE_ASM FALSE)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686|x86|x86_64|amd64|arm64")
    set(USE_ASM TRUE)
endif()

# Add the appropriate display implementation
if(USE_ASM)
    # Use assembly implementation
    if(WIN32)
        set(ASM_SOURCES display_functions_win.asm)
    else()
        set(ASM_SOURCES display_functions.asm)
    endif()

    # Explicitly compile the ASM file with nasm
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/display_functions.asm.o
        COMMAND nasm -f macho64 -o ${CMAKE_BINARY_DIR}/display_functions.asm.o ${CMAKE_SOURCE_DIR}/display_functions.asm
        DEPENDS ${CMAKE_SOURCE_DIR}/display_functions.asm
        COMMENT "Compiling display_functions.asm with NASM for macOS"
    )

    # Add the generated object file to the sources list
    list(APPEND SOURCES ${CMAKE_BINARY_DIR}/display_functions.asm.o)
else()
    list(APPEND SOURCES display_functions.c)
endif()

# Create executable
add_executable(microos ${SOURCES})

# Disable PIE linking to avoid relocation issues in the assembly object
if(APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-no_pie")
else()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -no-pie")
endif()

# Link libraries (AFTER add_executable)
if(APPLE)
    find_library(SDL2_LIB SDL2 HINTS /opt/homebrew/lib)
    find_library(SDL2_TTF_LIB SDL2_ttf HINTS /opt/homebrew/Cellar/sdl2_ttf/2.24.0/lib)
    target_link_libraries(microos ${SDL2_LIB} ${SDL2_TTF_LIB})
else()
    target_link_libraries(microos ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
endif()

# Installation
install(TARGETS microos DESTINATION bin)

# Package information
set(CPACK_PACKAGE_NAME "MicroOS")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "MicroOS Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A tiny graphical OS simulator")
include(CPack)

# Platform-specific settings
if(WIN32)
    file(GLOB SDL2_DLLS "${SDL2_DIR}/lib/x64/*.dll")
    file(GLOB SDL2_TTF_DLLS "${SDL2_TTF_DIR}/lib/x64/*.dll")
    file(COPY ${SDL2_DLLS} ${SDL2_TTF_DLLS} DESTINATION ${CMAKE_BINARY_DIR})
endif()
