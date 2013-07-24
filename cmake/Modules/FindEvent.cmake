# Find libevent
# ~~~~~~~~~~~~
#
# CMake module to search for libevent library
# (library event-driven network programming)
#
# If it's found it sets EVENT_FOUND to TRUE
# and following variables are set:
#    EVENT_INCLUDE_DIR
#    EVENT_LIBRARY

FIND_PATH(EVENT_INCLUDE_DIR event.h)
FIND_LIBRARY(EVENT_LIBRARY NAMES event libevent)

IF (EVENT_INCLUDE_DIR AND EVENT_LIBRARY)
   SET(EVENT_FOUND TRUE)
ENDIF (EVENT_INCLUDE_DIR AND EVENT_LIBRARY)


IF (EVENT_FOUND)

   IF (NOT EVENT_FIND_QUIETLY)
      MESSAGE(STATUS "Found LibEvent: ${EVENT_LIBRARY}")
   ENDIF (NOT EVENT_FIND_QUIETLY)

ELSE (EVENT_FOUND)

   IF (EVENT_FIND_REQUIRED)
     MESSAGE(FATAL_ERROR "Could not find LibEvent")
   ELSE (EVENT_FIND_REQUIRED)
     IF (NOT EVENT_FIND_QUIETLY)
        MESSAGE(STATUS "Could not find LibEvent")
     ENDIF (NOT EVENT_FIND_QUIETLY)
   ENDIF (EVENT_FIND_REQUIRED)

ENDIF (EVENT_FOUND)

