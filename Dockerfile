FROM gcr.io/google_containers/ubuntu-slim-amd64:0.14

RUN apt-get update && apt-get install -y --no-install-recommends \
  gcc \
  libc6-dev \
  make
  
RUN mkdir -p tcpecho
COPY src /tcpecho/src
COPY Makefile /tcpecho/Makefile
RUN cd tcpecho ; make

EXPOSE 3495

ENTRYPOINT ["/tcpecho/bin/tcpecho", "3495"]
