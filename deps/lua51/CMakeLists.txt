



set(LIB_NAME lua)

aux_source_directory(src LIB_SRCS)
#file(GLOB LIB_SRCS src/*.c)

if (UNIX)
##需要安装 readline
#find_library(libreadline NAMES readline /usr/lib /usr/lib/readline /usr/local/lib)
#if(NOT libreadline) 
#message(WARNING "cann't find library readline in linux")
    ADD_DEFINITIONS("-DLUA_USE_POSIX")  
#elseif(readline)
#ADD_DEFINITIONS("-DLUA_USE_LINUX -Wl,-E -ldl -lreadline -lncurses")  
#endif()

endif ()


add_library(${LIB_NAME} STATIC ${LIB_SRCS})

#install(TARGETS ${LIB_NAME}
# ARCHIVE DESTINATION ${PROJECT_SOURCE_DIR}/lib)


###  拷贝lua头文件到include/lua下
#set(DestIncludeDir ${PROJECT_SOURCE_DIR}/include/lua)

#set(SourIncludeFile 
#				src/lauxlib.h  
#				src/lua.h  
#				src/lua.hpp  
#				src/luaconf.h  
#				src/lualib.h
#)

#file(MAKE_DIRECTORY ${DestIncludeDir})


#file(COPY 
#${SourIncludeFile}
#DESTINATION ${DestIncludeDir}
#)