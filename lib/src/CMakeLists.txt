set (LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_LIST_DIR})

add_library (cmdparse SHARED cmdparse.cpp option.cpp info.cpp)

install (TARGETS cmdparse DESTINATION lib)
