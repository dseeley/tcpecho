# tcpecho
A simple, threaded TCP echo server.  Repeats back to you what you write.

Available as image on dockerhub:  https://hub.docker.com/repository/docker/dseeley/tcpecho

## Prerequisites:
+ c
+ gnu make
+ docker

## Build
c-compile
+ `make`
+ `make debug`

## Run locally (on `portnum`)
+ ` bin/tcpecho <portnum>`

### Or run in docker (on `SRV_PORT`):
+ `docker run -d --name tcpecho -e SRV_PORT=3495 dseeley/tcpecho:1.0.0`

## Create Docker image
Builds a docker image and pushes to DockerHub with the tag `latest`, and the (optional) `TAG`.
+ `[TAG=0.0.1] make dockerpush`

## k8s
A sample deployment and service manifest are included for reference.
