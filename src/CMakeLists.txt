add_executable(build build.cpp)
target_link_libraries(build
    MaskedVByte
    ${Boost_LIBRARIES}
)

add_executable(statistics statistics.cpp)
target_link_libraries(statistics
    MaskedVByte
    ${Boost_LIBRARIES}
)

add_executable(queries queries.cpp)
target_link_libraries(queries
    MaskedVByte
    ${Boost_LIBRARIES}
)
