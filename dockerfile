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
COPY --from=base__swicc /opt/swicc/build /opt/swicc/build
COPY --from=base__swicc /opt/swicc/test/data /opt/swicc/test/data

RUN set -eux; \
    rm -r /opt/swicc/build/swicc; \
    rm -r /opt/swicc/build/test; \
    rm -r /opt/swicc/build/cjson;

ENTRYPOINT [ "bash", "-c", "cp -r /opt/swicc/* /opt/out" ]
