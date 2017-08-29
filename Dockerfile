FROM gcr.io/google_containers/ubuntu-slim-amd64:0.14

RUN apt-get update && apt-get install -y --no-install-recommends \
  ca-certificates \
  git \
  build-essential

RUN git clone https://github.com/dseeley/tcpecho.git ; cd tcpecho ; make

EXPOSE 3495

ENTRYPOINT ["/tcpecho/bin/tcpecho", "3495"]
