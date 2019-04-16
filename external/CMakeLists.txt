EXECUTE_PROCESS(COMMAND git submodule update --init
                WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
                OUTPUT_QUIET
            )

include_directories(MaskedVByte/include)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -march=native")
add_library(MaskedVByte STATIC MaskedVByte/src/varintdecode.c
                               MaskedVByte/src/varintencode.c
)

include_directories(essentials/include)