
include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${LIBGC_INCLUDE_DIRECTORIES}
  ${GLIB_INCLUDE_DIRECTORIES}
)
add_definitions(-DMONO_BINDIR="${bindir}")

#AM_CPPFLAGS = \
#$(GLIB_CFLAGS)		\
#$(LIBGC_CPPFLAGS)	\
#-DMONO_BINDIR=\""$(bindir)"\"	\
#-I$(top_srcdir) \
#$(SHARED_CFLAGS)

set(OTHER_H
  access.h
  context.h
  error.h
  events.h
  handles.h
  io.h
  io-trace.h
  io-layer.h
  io-portability.h
  macros.h
  messages.h
  mutexes.h
  processes.h
  security.h
  semaphores.h
  sockets.h
  status.h
  threads.h
  timefuncs.h
  types.h
  uglify.h
  versioninfo.h
  wait.h
  wapi.h
  wapi-remap.h
)

set(OTHER_SRC
  access.h
  context.c
  context.h
  error.c
  error.h
  events.c
  events.h
  event-private.h
  handles.c
  handles.h
  handles-private.h
  io.c
  io.h
  io-portability.c
  io-portability.h
  io-private.h
  io-layer.h
  locking.c
  macros.h
  messages.c
  messages.h
  mutexes.c
  mutexes.h
  mutex-private.h
  posix.c
  processes.c
  processes.h
  process-private.h
  security.c
  security.h
  semaphores.c
  semaphores.h
  semaphore-private.h
  shared.c
  shared.h
  sockets.c
  sockets.h
  socket-private.h
  socket-wrappers.h
  status.h
  threads.h
  thread-private.h
  timefuncs.c
  timefuncs.h
  timefuncs-private.h
  types.h
  uglify.h
  versioninfo.c
  versioninfo.h
  wait.c
  wait.h
  wapi_glob.h
  wapi_glob.c
  wapi.h
  wapi-private.h
  wthreads.c
)

set(WINDOWS_H io-layer.h)

set(WINDOWS_SRC io-layer.h  io-layer-dummy.c)

if(HOST_WIN32)
  add_library(wapi STATIC ${WINDOWS_SRC} ${WINDOWS_H})
else()
  add_library(wapi STATIC ${OTHER_SRC} ${OTHER_H})
endif()
if(PLATFORM_DARWIN)
  target_link_libraries(wapi proc)
endif()
