FROM centos:7

WORKDIR /root

RUN yum install -y cmake && \
    yum install -y gcc && \
    

RUN mkdir src

ADD src/* src/
ADD CMakeLists.txt .

RUN mkdir build
RUN cd build
RUN cmake ..
RUN make -j4

# Открываем 80-й порт наружу
EXPOSE 80

# Запускаем наш сервер
CMD ./server.out 80