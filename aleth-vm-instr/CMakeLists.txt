add_executable(aleth-vm-instr main.cpp)

target_link_libraries(aleth-vm-instr PRIVATE ethereum evm ethashseal devcore Boost::program_options)

target_include_directories(aleth-vm-instr PRIVATE ../utils)

install(TARGETS aleth-vm-instr EXPORT alethTargets DESTINATION bin)
