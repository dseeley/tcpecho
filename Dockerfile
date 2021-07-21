FROM ubuntu:20.04

ENV TZ=Europe/London
ARG DEBIAN_FRONTEND=noninteractive

ENV SRV_PORT=3495

COPY src /tcpecho/src
COPY Makefile /tcpecho/Makefile

RUN apt-get update && apt-get install -y gcc libc6-dev make \
  && mkdir -p tcpecho ; cd tcpecho ; make \
  && apt-get remove -y gcc libc6-dev make \
  && apt-get -y autoremove \
  && apt-get -y clean

ENTRYPOINT /tcpecho/bin/tcpecho $SRV_PORT
#ENTRYPOINT ["/usr/bin/bash"]
