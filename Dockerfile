FROM gcr.io/google_containers/ubuntu-slim-amd64:0.14

COPY src /tcpecho/src
COPY Makefile /tcpecho/Makefile

RUN apt-get update && apt-get install -y --no-install-recommends gcc libc6-dev make \
  && mkdir -p tcpecho ; cd tcpecho ; make \
  && apt-get remove -y gcc libc6-dev make \
  && apt-get -y autoremove \
  && apt-get -y clean

EXPOSE 3495

ENTRYPOINT ["/tcpecho/bin/tcpecho", "3495"]
