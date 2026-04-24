cmake_minimum_required(VERSION 3.21)
project(nprt_studio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(NPRT_STUDIO_WITH_SCINTILLA "Enable Scintilla editor backend" ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql)

add_executable(nprt_studio
  src/main.cpp
  src/core/Application.cpp
  src/ui/MainWindow.cpp
  src/editor/EditorWidget.cpp
  src/workspace/WorkspaceService.cpp
  src/build/BuildService.cpp
  src/language/LspClient.cpp
  src/debug/DapClient.cpp
  src/extensions/PluginHost.cpp
  src/storage/StudioDatabase.cpp
)

target_include_directories(nprt_studio PRIVATE src)

if(NPRT_STUDIO_WITH_SCINTILLA)
  target_compile_definitions(nprt_studio PRIVATE NPRT_STUDIO_WITH_SCINTILLA=1)
endif()

target_link_libraries(nprt_studio PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::Sql
)
