
set(BinName saker-cli)

set(DepLibs 
     ${DepLibs}
     libhiredis.a
    )
    
set(BinSrcs 
    ../event/zmalloc.c
    ../event/adlist.c
    ../event/ae.c
    ../event/anet.c
    ../event/sds.c
    ${PROJECT_SOURCE_DIR}/deps/linenoise/linenoise.c
    saker-cli.c
)

include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/src/event")
include_directories("${PROJECT_SOURCE_DIR}/deps/linenoise")

link_directories(${PROJECT_SOURCE_DIR}/lib)   

add_executable(${BinName} ${BinSrcs})

target_link_libraries(${BinName} ${DepLibs})
