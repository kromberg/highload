FROM alpine:3.6

WORKDIR /root

RUN apk add --no-cache clang
RUN apk add --no-cache make
RUN apk add --no-cache cmake
RUN apk add --no-cache wget
RUN apk add --no-cache zlib-dev
RUN apk add --no-cache perl
RUN apk add --no-cache gcc
RUN apk add --no-cache g++
RUN wget https://nih.at/libzip/libzip-1.2.0.tar.gz
RUN tar xf libzip-1.2.0.tar.gz && \
    rm libzip-1.2.0.tar.gz && \
    cd libzip-1.2.0 && \
    ./configure && \
    make -j4 && \
    make install
RUN apk add --no-cache git
RUN git clone https://github.com/miloyip/rapidjson.git && \
    cd rapidjson && \
    git checkout tags/v1.1.0 && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j4 && \
    make install
RUN wget https://github.com/01org/tbb/archive/2017_U7.tar.gz

RUN tar xf 2017_U7.tar.gz && \
    rm 2017_U7.tar.gz
COPY tbb-mallinfo.patch tbb-2017_U7/
RUN cd tbb-2017_U7 && \
    git apply tbb-mallinfo.patch
RUN cd tbb-2017_U7 && \
    make compiler=clang

RUN mkdir -p highload/src
RUN mkdir -p highload/build

COPY src highload/src
ADD CMakeLists.txt highload/

RUN cd highload/build && CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake -DCMAKE_BUILD_TYPE=Release .. && make -j4 && make install
#RUN cd highload/build && CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake .. && make -j4 && make install

ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib:/usr/lib:/usr/lib64
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/root/tbb-2017_U7/build/linux_intel64_clang_cc6.3.0_libc_kernel4.9.36_release

EXPOSE 80

CMD server.out
