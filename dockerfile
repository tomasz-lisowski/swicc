# docker build --progress=plain . -t tomasz-lisowski/swicc:1.0.0 2>&1 | tee build.log;
# docker run -v ./build:/opt/swicc/build/host --tty --rm tomasz-lisowski/swicc:1.0.0;

FROM ubuntu:22.04 AS base

RUN set -eux; \
    apt-get -qq update; \
    apt-get -qq --yes dist-upgrade;

FROM base AS base__swicc
COPY . /opt/swicc
ENV DEP="cmake gcc gcc-multilib make"
RUN set -eux; \
    apt-get -qq --yes --no-install-recommends install ${DEP}; \
    cd /opt/swicc; \
    make clean; \
    make -j $(nproc) main-static test-static; \
    apt-get -qq --yes purge ${DEP};

FROM base
COPY --from=base__swicc /opt/swicc/build /opt/swicc/build/local
ENTRYPOINT [ "/bin/bash", "-c", "(cp -r /opt/swicc/build/local/*.a /opt/swicc/build/host) && (cp -r /opt/swicc/build/local/*.elf /opt/swicc/build/host) && (mkdir -p /opt/swicc/build/host/tmp)" ]
