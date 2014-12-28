
set(BinName ${PROJECT_NAME})

set(DepLibs 
     ${DepLibs}
     m
     libhiredis.a
     liblua.a
    )

set(EventSourcesFile 
    event/zmalloc.c
    event/adlist.c
    event/ae.c
    event/anet.c
    event/util.c
    event/sds.c
)

set(SubDir . utils core service sysinfo plugins proto luacompat)
list(APPEND BinSrcs ${EventSourcesFile})

foreach(TheDir ${SubDir})
    aux_source_directory(${TheDir} BinSrcs)
    if(NOT ${TheDir} STREQUAL ".")
        unset(TheSours)
        file(GLOB TheSours ${TheDir}/*.h)
        file(GLOB TheIncludes ${TheDir}/*.h)

        source_group(${TheDir} FILES ${TheSours} ${TheIncludes})
    endif() 
endforeach()

if (UNIX)
    aux_source_directory(sysinfo/linux BinSrcs)    
    file(GLOB SysInfoSours sysinfo/linux/*.[hc])
endif()

source_group(sysinfo FILES ${SysInfoSours})
include_directories(${PROJECT_SOURCE_DIR}/deps)
include_directories(${PROJECT_SOURCE_DIR}/deps/lua/src)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/event)

add_executable(${BinName} ${BinSrcs})

TARGET_LINK_LIBRARIES(${BinName}  ${DepLibs})
