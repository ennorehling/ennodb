cmake_minimum_required(VERSION 2.6)
project (ennodb-fcgi C)

IF(CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -Wall -Werror -Wno-unknown-pragmas -Wstrict-prototypes -Wpointer-arith -Wno-char-subscripts -Wno-long-long")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -DHAVE__BOOL")
ELSE(CMAKE_COMPILER_IS_GNUCC)
    MESSAGE(STATUS "Unknown compiler ${CMAKE_C_COMPILER_ID}")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

include_directories (${CRITBIT_INCLUDE_DIR})

set (FCGI_LIBRARY -lfcgi)
set (FCGI_SRC ennodb-fcgi.c)

add_executable(ennodb-fcgi ${FCGI_SRC})
target_link_libraries(ennodb-fcgi ${FCGI_LIBRARY})
