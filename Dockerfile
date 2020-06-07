FROM ubuntu:18.04

RUN apt update -y && apt install -y \
      cmake \
      g++ \
      git \
      python3 \
      curl \
      libssl-dev \
      zlib1g-dev \
      libcurl4-openssl-dev

WORKDIR /project

CMD ls && scripts/rebuild-target cli s3benchmark_cli_exec cmake-build-cli-docker
