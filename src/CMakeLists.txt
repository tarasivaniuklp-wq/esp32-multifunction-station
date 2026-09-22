# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(
    SRCS 
        "main.cpp"
        "InputManager.cpp"
        "DisplayManager.cpp"
        "OLED_display.cpp"
        "Pages.cpp"
        "NetworkManager.cpp"
        "SensorManager.cpp"
        "AlarmControler.cpp"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        driver 
        freertos 
        log
)