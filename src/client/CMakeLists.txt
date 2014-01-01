
set(EXE_NAME saker-cli)

set(EXE_SRCS 
../event/zmalloc.c
../event/adlist.c
../event/ae.c
../event/anet.c
../utils/sds.c
${PROJECT_SOURCE_DIR}/deps/linenoise/linenoise.c
saker-cli.c
)

if(CMAKE_SYSTEM MATCHES "Windows")
list(APPEND EXE_SRCS ../event/win32_wsiocp.c)
list(APPEND EXE_SRCS ../event/win32fixes.c)
endif()

include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/deps/hiredis")
include_directories("${PROJECT_SOURCE_DIR}/deps/linenoise")

link_directories(${PROJECT_SOURCE_DIR}/lib)   

add_executable(${EXE_NAME} ${EXE_SRCS})

#set(LIB_VAR lua)

if (UNIX)
  set(LIB_VAR ${LIB_VAR} m)
elseif (WIN32)
  set(LIB_VAR 
  #set(LIB_VAR lua)
     ws2_32.lib
     shell32.lib
  )
endif()
set(LIB_VAR hiredis)

if (UNIX)
    set(LIB_VAR ${LIB_VAR} m)
endif ()

TARGET_LINK_LIBRARIES(${EXE_NAME}  ${LIB_VAR})

