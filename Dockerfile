FROM beamdog/nwserver:latest as nwserver
LABEL maintainers "jakobknutsen@gmail.com"

FROM nwneetools/nwnsc-docker-builder:071c269 as nwnsc-builder
# Keep the legacy Linux compiler and ABI, but update the parser generator.
ADD https://ftp.gnu.org/gnu/bison/bison-3.8.2.tar.gz /tmp/bison.tar.gz
RUN echo "06c9e13bdf7eb24d4ceb6b59205a4f67c2c7e7213119644430fe82fbd14a0abb  /tmp/bison.tar.gz" | sha256sum -c - \
    && tar -xzf /tmp/bison.tar.gz -C /tmp \
    && cd /tmp/bison-3.8.2 \
    && /hbb_exe/activate-exec /bin/bash -c 'FORCE_UNSAFE_CONFIGURE=1 ./configure --prefix=/opt/bison --disable-nls && make -j2 && make install' \
    && rm -rf /tmp/bison.tar.gz /tmp/bison-3.8.2
ENV PATH="/opt/bison/bin:${PATH}"

FROM nwnsc-builder as nwnsc
COPY ./ /tmp/nwnsc/
WORKDIR /tmp/nwnsc
RUN /hbb_exe/activate-exec /bin/bash -c 'cmake -DBISON_EXECUTABLE=/opt/bison/bin/bison . && make -j2 && ctest --output-on-failure'

FROM i386/ubuntu:latest
COPY --from=nwnsc /tmp/nwnsc/nwnsc/nwnsc /usr/local/bin/
COPY --from=nwserver /nwn/data /nwn/data
WORKDIR /tmp
ENTRYPOINT ["nwnsc", "-on", "/nwn/data"]
CMD ["*.nss"]
