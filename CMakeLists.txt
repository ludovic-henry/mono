cmake_minimum_required(VERSION 2.8.11)
# project(mono)

# -DCFLAGS_FOR_EGLIB:STRING="$(CFLAGS_FOR_EGLIB)" \
# -DCROSS_COMPILING:BOOL=$(cross_compiling) \
# -DHOST_WIN32:BOOL=$(host_win32) \
# -DHOST_DARWIN:BOOL=$(host_darwin) \
# -DTARGET_OSX:BOOL=$(target_osx) \
# -DTARGET_HAIKU:BOOL=$(target_haiku)

add_subdirectory(eglib)
