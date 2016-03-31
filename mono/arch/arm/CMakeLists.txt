# TODO: Why not just push these generated files to remove the dependency on bash?
# TODO: The input files haven't changed in over a decade.

execute_process(COMMAND "bash ./dpiops.sh" OUTPUT_FILE arm_dpimacros.h)
execute_process(COMMAND "bash ./vfpops.sh" OUTPUT_FILE arm_vfpmacros.h)
