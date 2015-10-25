
list (APPEND SOURCES
	closure.c
	error.c
	lalr.c
	lr0.c
	main.c
	mkpar.c
	output.c
	reader.c
	symtab.c
	verbose.c
	warshall.c
)

add_executable (jay ${SOURCES})
set_target_properties (jay PROPERTIES COMPILE_FLAGS "-DSKEL_DIRECTORY=\"\\\"${CMAKE_INSTALL_PREFIX}/share/jay\\\"\"")
