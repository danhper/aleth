set(
    sources
    AccountManager.cpp AccountManager.h
    main.cpp
    MinerAux.cpp MinerAux.h
)

add_executable(aleth ${sources})
set(private_link_libraries
    ethereum ethashseal evm web3jsonrpc webthree devcore Boost::program_options
)
if(MEASURE_GAS)
    set(private_link_libraries ${private_link_libraries} Boost::iostreams evmanalysis)
endif()
target_link_libraries(aleth PRIVATE ${private_link_libraries})

target_include_directories(aleth PRIVATE ../utils)

if(MINIUPNPC)
    target_compile_definitions(aleth PRIVATE ETH_MINIUPNPC)
endif()

install(TARGETS aleth DESTINATION bin)
