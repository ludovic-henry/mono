
add_custom_command(OUTPUT opcode.def
  COMMAND perl "${CMAKE_CURRENT_SOURCE_DIR}/make-opcodes-def.pl}" "${CMAKE_CURRENT_SOURCE_DIR}/cil-opcodes.xml" opcode.def
  DEPENDS make-opcodes-def.pl cil-opcodes.xml )
