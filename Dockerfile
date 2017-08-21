# CentOS 7
FROM centos:7

WORKDIR /root

RUN yum install -y epel-release
RUN yum install -y clang
RUN yum install -y make
RUN yum install -y cmake
RUN yum install -y wget
RUN yum install -y zlib-devel
RUN yum install -y perl
RUN yum install -y gcc
RUN yum install -y gcc-c++
RUN wget https://nih.at/libzip/libzip-1.2.0.tar.gz
RUN tar xf libzip-1.2.0.tar.gz && \
    rm libzip-1.2.0.tar.gz && \
    cd libzip-1.2.0 && \
    ./configure && \
    make -j4 && \
    make install
RUN yum install -y git
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
    rm 2017_U7.tar.gz && \
    cd tbb-2017_U7 && \
    gmake

RUN git clone https://github.com/gperftools/gperftools.git
RUN yum install -y autoconf
RUN yum install -y automake
RUN yum install -y libtool
RUN cd gperftools && ./autogen.sh && ./configure && make -j4 && make install

RUN mkdir -p highload/src
RUN mkdir -p highload/build

COPY src highload/src
ADD CMakeLists.txt highload/

RUN cd highload/build && CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake -DCMAKE_BUILD_TYPE=Release .. && make -j4 && make install
#RUN cd highload/build && CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake .. && make -j4 && make install

ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib:/usr/lib:/usr/lib64
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/root/tbb-2017_U7/build/linux_intel64_gcc_cc4.8.5_libc2.17_kernel4.9.36_release

EXPOSE 80

#RUN yum install -y graphviz
#RUN yum install -y ghostscript

CMD CPUPROFILE=/tmp/prof/prof.out server.out
#CMD server.out