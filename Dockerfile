FROM ubuntu:18.04

RUN apt update && apt install \
      cmake \
      g++ \
      ssl-dev

WORKDIR /project

CMD scripts/rebuild-target
