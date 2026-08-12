cmake_minimum_required(VERSION 3.20)
project(AnalisiVideoSperimentaleAvanzata LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- CUDA opzionale ---
option(ENABLE_CUDA "Abilita supporto CUDA" ON)
if(ENABLE_CUDA)
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
endif()

# --- Qt6 ---
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

# --- OpenCV ---
find_package(OpenCV REQUIRED)

# --- Sorgenti ---
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/mainwindow_addons.cpp
    src/Analysisworker.cpp
    src/DahuaCameraControl.cpp
    src/DetectionPreset.cpp
    src/FfmpegReader.cpp
    src/LiveFrameProcessor.cpp
    src/RoiIO.cpp
    src/RoiScene.cpp
    src/SimpleTracker.cpp
    src/analyzerCli.cpp
)

set(HEADERS
    src/MainWindow.h
    src/Analysisworker.h
    src/DahuaCameraControl.h
    src/DetectionPreset.h
    src/FfmpegReader.h
    src/LiveFrameProcessor.h
    src/RoiIO.h
    src/RoiScene.h
    src/SimpleTracker.h
    src/cuda_pipe.h
    src/cuda_stub.cpp
    src/threadQueue.h
)

# --- CUDA sorgenti ---
if(ENABLE_CUDA)
    list(APPEND SOURCES src/cuda_pipe.cu)
else()
    list(APPEND SOURCES src/cuda_stub.cpp)
endif()

# --- Eseguibile ---
add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    ${HEADERS}
)

# --- Include dirs ---
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)

# --- Link libraries ---
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    ${OpenCV_LIBS}
)

# --- CUDA flags ---
if(ENABLE_CUDA)
    find_package(CUDAToolkit)
    if(CUDAToolkit_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE
            CUDA::cudart
            CUDA::cublas
        )
        target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_CUDA)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            CUDA_SEPARABLE_COMPILATION ON
        )
    endif()
endif()
