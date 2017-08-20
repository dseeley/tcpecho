FROM gcr.io/google_containers/ubuntu-slim-amd64:0.14

RUN apt-get update && apt-get install -y --no-install-recommends \
  build-essential

RUN mkdir -p tcpecho

EXPOSE 3495

COPY src /tcpecho/src
COPY Makefile /tcpecho/Makefile
RUN cd tcpecho ; make

ENTRYPOINT ["/tcpecho/bin/tcpecho", "3495"]
