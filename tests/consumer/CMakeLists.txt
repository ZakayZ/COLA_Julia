cmake_minimum_required(VERSION 3.20)
project(COLAJuliaConsumer LANGUAGES CXX)

find_package(COLA CONFIG REQUIRED)
find_package(COLA_Julia CONFIG REQUIRED HINTS "${COLA_DIR}")
enable_testing()

add_executable(linked linked.cc)
target_link_libraries(linked PRIVATE COLA_Julia::COLA_Julia)
add_test(NAME linked COMMAND linked)

add_executable(plugin ../julia_filters_test.cc)
target_link_libraries(plugin PRIVATE COLA)
add_test(NAME plugin COMMAND plugin "$<TARGET_FILE_DIR:COLA_Julia::COLA_Julia>/..")
