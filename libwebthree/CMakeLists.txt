add_library(webthree WebThree.cpp WebThree.h)
target_include_directories(webthree PRIVATE ../utils)

set(public_link_libraries ethereum p2p devcore)
if (MEASURE_GAS)
  set (public_link_libraries ${public_link_libraries} evmanalysis)
endif()

target_link_libraries(
  webthree
  PUBLIC ${public_link_libraries}
)
