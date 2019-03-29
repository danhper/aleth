FROM ubuntu:18.04
ARG cflags=""
ARG cmake_flags=""

RUN apt-get update
RUN apt-get install -y git libleveldb-dev clang valgrind cmake build-essential lcov

COPY . /aleth
WORKDIR /aleth
RUN git submodule update --init
RUN mkdir /aleth/build
RUN cd /aleth/build && \
  CLFAGS=$cflags CXXFLAGS=$cflags \
  cmake .. $cmake_flags
RUN cd /aleth/build && make -j5
RUN ln -sr /aleth/build/aleth/aleth /usr/local/bin/aleth
ENTRYPOINT ["/usr/local/bin/aleth"]
