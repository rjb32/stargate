# Each subdirectory under designs/ is a regress test for one
# externally-fetched RTL design. Files are pulled from upstream into
# a build-tree cache on first run and re-used thereafter.
add_subdirectory(picorv32)
add_subdirectory(sha256)
add_subdirectory(aes)
add_subdirectory(darkriscv)
add_subdirectory(openmsp430)
add_subdirectory(mips_neelkshah)
add_subdirectory(ddr2_controller)
add_subdirectory(iwls2005)
add_subdirectory(ibex)
add_subdirectory(mini68k)
