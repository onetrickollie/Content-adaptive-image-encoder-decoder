cmake_minimum_required(VERSION 3.10)

project(ImageDisplay)

set(CMAKE_CXX_STANDARD 17)

# Add source files
# Note: manifest/MyImageApplication.rc is Windows-only, ignored on Mac/Linux
set(SOURCES
    src/Main.cpp
    src/Image.cpp
    src/DCT.cpp
    src/Encoder.cpp
    src/Decoder.cpp
)

if(WIN32)
    # On Windows: build wxWidgets from bundled source
    set(wxBUILD_SHARED OFF)
    list(APPEND SOURCES manifest/MyImageApplication.rc)
    add_subdirectory(dependency/wxWidgets)
    set(WX_LIBS wx::core wx::base)
else()
    # On Mac/Linux: use system-installed wxWidgets (e.g. via Homebrew)
    # Install on Mac with: brew install wxwidgets
    find_package(wxWidgets REQUIRED COMPONENTS core base)
    include(${wxWidgets_USE_FILE})
    set(WX_LIBS ${wxWidgets_LIBRARIES})
endif()

# Create executable
add_executable(myProgram ${SOURCES})

# Add src/ so that #include "Image.h" etc. resolve correctly
target_include_directories(myProgram PRIVATE src)

# X11 required on Linux
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
endif()

# Link libraries
target_link_libraries(myProgram ${WX_LIBS} ${X11_LIBRARIES})