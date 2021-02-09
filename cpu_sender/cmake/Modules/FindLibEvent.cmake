
# - Try to find the LibEvent config processing library
# Once done this will define
#
# LIBEVENT_FOUND - System has LibEvent
# LIBEVENT_INCLUDE_DIR - the LibEvent include directory
# LIBEVENT_LIBRARIES 0 The libraries needed to use LibEvent
# Use PATHS in each of the find* to point to the local libevent install path

find_path     (LIBEVENT_INCLUDE_DIR
        NAMES event.h
        PATHS "/home/user/local/include")
find_library  (LIBEVENT_LIBRARY
        NAMES event
        PATHS "/home/user/local/lib")
find_library  (LIBEVENT_CORE
        NAMES event_core
        PATHS "/home/user/local/lib")
find_library  (LIBEVENT_EXTRA
        NAMES event_extra
        PATHS "/home/user/local/lib")
if (NOT EVHTP_DISABLE_EVTHR)
    find_library (LIBEVENT_THREAD
            NAMES event_pthreads
            PATHS "/home/user/local/lib")
endif()
if (NOT EVHTP_DISABLE_SSL)
    find_library (LIBEVENT_SSL
            NAMES event_openssl
            PATHS "/home/user/local/lib")
endif()
include (FindPackageHandleStandardArgs)
set (LIBEVENT_INCLUDE_DIRS ${LIBEVENT_INCLUDE_DIR})
set (LIBEVENT_LIBRARIES
        ${LIBEVENT_LIBRARY}
        ${LIBEVENT_SSL}
        ${LIBEVENT_CORE}
        ${LIBEVENT_EXTRA}
        ${LIBEVENT_THREAD}
        ${LIBEVENT_EXTRA})
find_package_handle_standard_args (LIBEVENT DEFAULT_MSG LIBEVENT_LIBRARIES LIBEVENT_INCLUDE_DIR)
mark_as_advanced(LIBEVENT_INCLUDE_DIRS LIBEVENT_LIBRARIES)