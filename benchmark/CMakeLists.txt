add_executable(profile profile.cpp)
target_link_libraries(profile
    MaskedVByte
    ${Boost_LIBRARIES}
    )

add_executable(profile_ef profile_ef.cpp)
target_link_libraries(profile_ef
    ${Boost_LIBRARIES}
    )
