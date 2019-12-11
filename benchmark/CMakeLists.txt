add_executable(profile profile.cpp)
target_link_libraries(profile
    MaskedVByte
    )

add_executable(profile_ef profile_ef.cpp)
target_link_libraries(profile_ef
    )
