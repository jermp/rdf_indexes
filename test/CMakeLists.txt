add_executable(check_index check_index.cpp)
target_link_libraries(check_index
    MaskedVByte
    ${Boost_LIBRARIES}
)

add_executable(check_queries check_queries.cpp)
target_link_libraries(check_queries
    MaskedVByte
    ${Boost_LIBRARIES}
)

add_executable(check_find check_find.cpp)
target_link_libraries(check_find
    MaskedVByte
    ${Boost_LIBRARIES}
)

add_executable(test_queries test_queries.cpp)
target_link_libraries(test_queries
    MaskedVByte
    ${Boost_LIBRARIES}
)