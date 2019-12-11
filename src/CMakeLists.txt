add_executable(build build.cpp)
target_link_libraries(build
    MaskedVByte
)

add_executable(statistics statistics.cpp)
target_link_libraries(statistics
    MaskedVByte
)

add_executable(queries queries.cpp)
target_link_libraries(queries
    MaskedVByte
)

add_executable(build_permutation build_permutation.cpp)
target_link_libraries(build_permutation
    MaskedVByte
)