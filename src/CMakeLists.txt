



#find_package(lua REQUIRED)

set(EXE_NAME ${PROJECT_NAME})

set(EventSourcesFile 
event/zmalloc.c
event/adlist.c
event/ae.c
event/anet.c
event/util.c
)

if(CMAKE_SYSTEM MATCHES "Windows")
list(APPEND EventSourcesFile event/win32_wsiocp.c)
list(APPEND EventSourcesFile event/win32fixes.c)
endif()


set(SubDir . utils core service sysinfo plugins proto luacompat)



list(APPEND EXE_SRCS ${EventSourcesFile})


foreach(TheDir ${SubDir})
  aux_source_directory(${TheDir} EXE_SRCS)

  if(NOT ${TheDir} STREQUAL ".")
    unset(TheSours)
    file(GLOB TheSours ${TheDir}/*.h)
    file(GLOB TheIncludes ${TheDir}/*.h)

    source_group(${TheDir} FILES ${TheSours} ${TheIncludes})
  endif() 
endforeach()


if (UNIX)
    aux_source_directory(sysinfo/linux EXE_SRCS)    
    file(GLOB SysInfoSours sysinfo/linux/*.[hc])
elseif (WIN32)
    aux_source_directory(sysinfo/windows EXE_SRCS)
    file(GLOB SysInfoSours sysinfo/windows/*.[hc])
endif()

source_group(sysinfo FILES ${SysInfoSours})
include_directories("${PROJECT_SOURCE_DIR}/deps")
include_directories("${PROJECT_SOURCE_DIR}/deps/lua/src")
include_directories("${PROJECT_SOURCE_DIR}/src")


link_directories(${PROJECT_SOURCE_DIR}/lib)   

#add_dependencies(${EXE_NAME} lua)

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
set(LIB_VAR lua hiredis)

if (UNIX)
    set(LIB_VAR ${LIB_VAR} m)
endif ()

TARGET_LINK_LIBRARIES(${EXE_NAME}  ${LIB_VAR})

