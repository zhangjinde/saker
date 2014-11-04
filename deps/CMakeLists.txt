
ADD_SUBDIRECTORY(lua)

ADD_SUBDIRECTORY(hiredis)

add_custom_command(  
  OUTPUT ${CMAKE_SOURCE_DIR}/lib/liblua.a  
  COMMAND  ${CMAKE_SOURCE_DIR}/deps/lua/make ansi
  DEPENDS ${CMAKE_SOURCE_DIR}/deps/lua/Makefile
)

add_custom_command(  
  OUTPUT ${CMAKE_SOURCE_DIR}/lib/libhiredis.a  
  COMMAND  ${CMAKE_SOURCE_DIR}/deps/hiredis/make static DEBUG=""
  DEPENDS ${CMAKE_SOURCE_DIR}/deps/hiredis/Makefile
)

