# CentOS 7
FROM centos:7

WORKDIR /root

RUN yum install -y gcc
RUN yum install -y gcc-c++
RUN yum install -y make
RUN yum install -y cmake
RUN yum install -y wget
RUN yum install -y zlib-devel
RUN yum install -y perl
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

RUN mkdir -p highload/src
RUN mkdir -p highload/build

# testing
RUN mkdir -p /tmp/data
ADD data.zip /tmp/data/

COPY src highload/src
ADD CMakeLists.txt highload/

RUN cd highload/build && cmake .. && make -j4 && make install

ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib:/usr/lib:/usr/lib64

EXPOSE 80

CMD server.out