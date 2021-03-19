FROM ubuntu:20.10

RUN apt-get update -y && apt-get install -y \
      git \
      python3 \
      g++ \
      ninja-build \
      cmake \
      libssl-dev \
      zlib1g-dev \
      curl \
      liburing1 \
      libcurl4 \
      libcurl4-openssl-dev \
      libcurl3-gnutls

WORKDIR /project

CMD ls && scripts/rebuild-target cli s3benchmark_cli_exec cmake-build-cli-docker
