file(GLOB sources "*.cpp" "*.h")

add_library(ethereum ${sources})

target_include_directories(ethereum PRIVATE "${UTILS_INCLUDE_DIR}")

set(public_link_libraries
  evm ethcore p2p devcrypto devcore 
)

if (MEASURE_GAS)
  set (public_link_libraries ${public_link_libraries} evmanalysis)
endif()

target_link_libraries(
  ethereum
  PUBLIC ${public_link_libraries}
  PRIVATE jsoncpp_lib_static Snappy::snappy Boost::fiber Boost::context
)
