FROM ubuntu:latest
LABEL maintainers "jakobknutsen@gmail.com & glorwinger"
WORKDIR /tmp
COPY ./ ./nwnsc/
RUN buildDeps="build-essential cmake bison" \
    && runDeps="g++-multilib" \
    && apt-get update \
    && apt-get install -y --no-install-recommends $buildDeps $runDeps \
    && cd nwnsc \
    && cmake . \
    && make \
    && mv nwnsc/nwnsc /usr/local/bin \
    && cd /tmp \
    && rm -rf /tmp/* \
    && apt-get purge $buildDeps -y \
    && apt-get autoremove -y \
    && apt-get clean -y \
    && rm -r /var/lib/apt/lists /var/cache/apt
ENTRYPOINT [ "nwnsc"]
CMD [ "*.nss" ]
