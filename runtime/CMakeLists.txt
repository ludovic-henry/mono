set(BUILD_PROFILES)

if(INSTALL_4_X)
  set(BUILD_PROFILES ${BUILD_PROFILES} binary_reference_assemblies net_4_x xbuild_12 xbuild_14)
  set(AL_PROFILE net_4_x)
endif()

if(INSTALL_MONODROID)
  set(BUILD_PROFILES ${BUILD_PROFILES} monodroid)
endif()

if(INSTALL_MONOTOUCH)
  set(BUILD_PROFILES ${BUILD_PROFILES} monotouch monotouch_runtime)
endif()

if(INSTALL_MONOTOUCH_WATCH)
  set(BUILD_PROFILES ${BUILD_PROFILES} monotouch_watch monotouch_watch_runtime)
endif()

if(INSTALL_MONOTOUCH_TV)
  set(BUILD_PROFILES ${BUILD_PROFILES} monotouch_tv monotouch_tv_runtime)
endif()

if(INSTALL_MOBILE_STATIC)
  set(BUILD_PROFILES ${BUILD_PROFILES} mobile_static)
endif()

if(INSTALL_XAMMAC)
  set(BUILD_PROFILES ${BUILD_PROFILES} xammac xammac_net_4_5)
endif()

string(REPLACE ";" " " BUILD_PROFILES "${BUILD_PROFILES}")

if(BUILD_MCS)
#  cd $(mcs_topdir) && $(MAKE) NO_DIR_CHECK=1 PROFILES='$(build_profiles)' RUNTIME_HAS_CONSISTENT_GACDIR=yes prefix=$(prefix) install-profiles

  file(COPY "${CMAKE_SOURCE_DIR}/data/net_2_0/machine.config" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/etc/mono/2.0")
  file(COPY "${CMAKE_SOURCE_DIR}/data/net_4_0/machine.config" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/etc/mono/4.0")
  file(COPY "${CMAKE_SOURCE_DIR}/data/net_4_5/machine.config" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/etc/mono/4.5")
#etc/mono/2.0/machine.config: $(top_srcdir)/data/net_2_0/machine.config
#etc/mono/2.0/web.config: $(top_srcdir)/data/net_2_0/web.config
#etc/mono/browscap.ini: $(top_srcdir)/data/browscap.ini
#etc/mono/2.0/Browsers/Compat.browser: $(top_srcdir)/data/Browsers/Compat.browser
#etc/mono/4.0/Browsers/Compat.browser: $(top_srcdir)/data/Browsers/Compat.browser
#etc/mono/4.5/Browsers/Compat.browser: $(top_srcdir)/data/Browsers/Compat.browser
#etc/mono/4.0/machine.config: $(top_srcdir)/data/net_4_0/machine.config
#etc/mono/4.0/web.config: $(top_srcdir)/data/net_4_0/web.config
#etc/mono/4.5/machine.config: $(top_srcdir)/data/net_4_5/machine.config
#etc/mono/4.5/web.config: $(top_srcdir)/data/net_4_5/web.config

  # NOTE: We pass runtime_dep='' to make to override the runtime_dep value set in mcs/class/aot-compiler/Makefile which
  # assumes there's a mono executable in mono/mini/.
  # TODO: Couldn't get assembly signing to work so disable it with NO_SIGN_ASSEMBLY=1
  ExternalProject_Add(mcs
    SOURCE_DIR ${MCS_TOPDIR}
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND bash -c "CMAKE_BINARY_DIR='${CMAKE_BINARY_DIR}' make -w V=1 NO_SIGN_ASSEMBLY=1 NO_DIR_CHECK=1 PROFILES='${BUILD_PROFILES}' CC=${CMAKE_C_COMPILER} runtime_dep='' all-profiles"
    INSTALL_COMMAND bash -c "CMAKE_BINARY_DIR='${CMAKE_BINARY_DIR}' make -w V=1 NO_SIGN_ASSEMBLY=1 NO_DIR_CHECK=1 PROFILES='${BUILD_PROFILES}' RUNTIME_HAS_CONSISTENT_GACDIR=yes prefix='${CMAKE_BINARY_DIR}/mcs' install-profiles"
    BUILD_IN_SOURCE 1
    LOG_BUILD 1
  )
  if(SUPPORT_SGEN)
    add_dependencies(mcs mono-sgen)
  else()
    add_dependencies(mcs mono-boehm)
  endif()

#MAKE_FLAGS=$(if $(V),,--no-print-directory -s)

#all-local: $(SUPPORT_FILES) $(TEST_SUPPORT_FILES)
#if test -w $(mcs_topdir); then :; else chmod -R +w $(mcs_topdir); fi
#cd $(mcs_topdir) && $(MAKE) $(MAKE_FLAGS) NO_DIR_CHECK=1 PROFILES='$(build_profiles)' CC='$(CC)' all-profiles


endif()


#ExternalProject_Add(extlibgc
#         URL file://${CMAKE_SOURCE_DIR}/libgc
#         UPDATE_COMMAND ""
#         CONFIGURE_COMMAND bash -c "CFLAGS='${CFLAGS_FOR_LIBGC} -I${CMAKE_SOURCE_DIR} -I${CMAKE_BINARY_DIR}' CPPFLAGS='${CPPFLAGS_FOR_LIBGC} -I${CMAKE_SOURCE_DIR} -I${CMAKE_BINARY_DIR}' ./configure --disable-embed-check --with-libgc-threads=${LIBGC_THREADS} ${LIBGC_CONFIGURE_ARGS} --prefix=${CMAKE_BINARY_DIR}/gc"
#         INSTALL_COMMAND ""
#         BUILD_IN_SOURCE 1
#         )
#add_library(monogc STATIC IMPORTED)
#set_target_properties(monogc PROPERTIES IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/extlibgc-prefix/src/extlibgc/.libs/libmonogc.a")
#add_dependencies(monogc extlibgc)
