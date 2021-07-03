# tcpecho
A simple, threaded TCP echo server.  Repeats back to you what you write.

Available as image on dockerhub:  https://hub.docker.com/repository/docker/dseeley/tcpecho

## Prerequisites:
+ gcc and gnu make
  + `apt-get install -y gcc libc6-dev make`
+ docker (if making docker image) 

## Build
c-compile
+ `make`
+ `make debug`

## Run locally (on `portnum`)
+ ` bin/tcpecho <portnum>`

## Docker

### Create docker image
Builds a docker image with the tag `latest`, and the (optional) `TAG` version.
```bash
[TAG=0.0.1] make dockerbuild
```

### Create docker image and push to dockerhub
Builds a docker image and pushes to DockerHub with the tag `latest`, and the (optional) `TAG` version.
```bash
[TAG=0.0.1] make dockerpush
```

### Run in docker (on `SRV_PORT`):
```bash
docker run -d --name tcpecho -e SRV_PORT=3495 dseeley/tcpecho:latest
```

### Run in k8s
Also bundled a deployment and service manifests for reference.
