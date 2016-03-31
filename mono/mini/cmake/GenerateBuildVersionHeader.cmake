if(NOT FILE)
  message(FATAL_ERROR "Missing -DFILE=...")
endif()

string(TIMESTAMP TS UTC)
file(WRITE "${FILE}" "const char *build_date = \"${TS}\";\n")
