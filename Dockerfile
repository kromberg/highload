# CentOS 7
FROM centos:7

WORKDIR /root

RUN yum install -y gcc
RUN yum install -y gcc-c++
RUN yum install -y make
RUN yum install -y cmake
RUN yum install -y zlib-devel 

RUN mkdir -p highload/src
RUN mkdir -p highload/build

COPY src highload/src
ADD CMakeLists.txt highload/

RUN cd highload/build && cmake .. && make -j4 && make install

EXPOSE 80

CMD server.out