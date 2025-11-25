set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_REVISION 0)
set(VERSION_STATUS "-dev")
string(TIMESTAMP VERSION_YEAR "%Y")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(VERSION_BUILD ".dbg" )
endif()
 
# La lógica de Git ha sido eliminada. La versión ahora se controla manualmente.
set(VERSION_HASH "")