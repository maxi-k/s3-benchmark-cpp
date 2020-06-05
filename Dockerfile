FROM ubuntu:18.04

RUN apt update -y && apt install -y \
      cmake \
      g++ \
      libssl-dev \
      git \
      python3 \
      zlib1g-dev \
      curl \
      libcurl4-openssl-dev

WORKDIR /project

CMD ls && scripts/rebuild-target
