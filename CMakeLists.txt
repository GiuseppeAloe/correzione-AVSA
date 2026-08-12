cmake_minimum_required(VERSION 3.20)

# CUDA va dichiarato qui solo se disponibile; usiamo CXX base
# e aggiungiamo CUDA condizionalmente dopo
project(AnalisiVideoSperimentaleAvanzata LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Qt6 ---
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

# --- OpenCV ---
find_package(OpenCV REQUIRED)

# --- CUDA opzionale ---
option(ENABLE_CUDA "Abilita supporto CUDA" ON)
if(ENABLE_CUDA)
    find_package(CUDAToolkit QUIET)
    if(CUDAToolkit_FOUND)
        enable_language(CUDA)
        set(CMAKE_CUDA_STANDARD 17)
        set(CMAKE_CUDA_STANDARD_REQUIRED ON)
        set(USE_CUDA TRUE)
        message(STATUS "CUDA trovato: ${CUDAToolkit_VERSION}")
    else()
        set(USE_CUDA FALSE)
        message(WARNING "CUDA non trovato, build senza CUDA")
    endif()
else()
    set(USE_CUDA FALSE)
endif()

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

if(USE_CUDA)
    list(APPEND SOURCES src/cuda_pipe.cu)
else()
    list(APPEND SOURCES src/cuda_stub.cpp)
endif()

# --- Eseguibile ---
add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

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

if(USE_CUDA)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        CUDA::cudart
        CUDA::cublas
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_CUDA)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
    )
endif()
