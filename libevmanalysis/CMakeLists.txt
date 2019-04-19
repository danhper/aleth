set(sources
    SystemUsageStatCollector.h SystemUsageStatCollector.cpp
    InstructionStats.h InstructionStats.cpp
    BenchmarkResults.h BenchmarkResults.cpp
    AnalysisEnv.h AnalysisEnv.cpp
    StreamWrapper.h StreamWrapper.cpp
)

add_library(evmanalysis ${sources})

target_link_libraries(
    evmanalysis
    PUBLIC evm
    PRIVATE Boost::iostreams Boost::thread
)
