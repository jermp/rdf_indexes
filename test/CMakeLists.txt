add_executable(check_index check_index.cpp)
target_link_libraries(check_index
    MaskedVByte
)

add_executable(check_queries check_queries.cpp)
target_link_libraries(check_queries
    MaskedVByte
)

add_executable(check_find check_find.cpp)
target_link_libraries(check_find
    MaskedVByte
)
