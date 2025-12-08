FROM almalinux:8 AS build

RUN dnf update -y && \
    dnf install -y \
    gcc-c++ \
    cmake \
    make \
    zlib-devel \
    libevent-devel \
    git \
    && dnf clean all

WORKDIR /app

COPY . .

RUN make build

FROM almalinux:8 AS production

RUN dnf update -y && \
    dnf install -y \
    zlib \
    libevent \
    && dnf clean all

COPY --from=build /app/build/merkle_tree /usr/local/bin/merkle_tree

ENTRYPOINT ["merkle_tree"]
