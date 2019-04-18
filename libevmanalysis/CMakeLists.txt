set(sources
    SystemUsageStatCollector.cpp SystemUsageStatCollector.h
    InstructionStats.cpp InstructionStats.h
    BenchmarkResults.cpp BenchmarkResults.h
)

add_library(evmanalysis ${sources})

# target_link_libraries(
#     evmanalysis
#     PUBLIC ethcore devcore evmc::evmc
#     PRIVATE aleth-interpreter
# )
