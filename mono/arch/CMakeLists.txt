if(ARM)
  # arm needs to build some stuff even in JIT mode
  add_subdirectory({ARCH_TARGET})
endif()
