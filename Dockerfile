FROM beamdog/nwserver:latest as nwserver
LABEL maintainers "jakobknutsen@gmail.com"

FROM nwneetools/nwnsc-docker-builder:latest as nwnsc
COPY ./ /tmp/nwnsc/
WORKDIR /tmp/nwnsc
RUN /hbb_exe/activate-exec cmake . && make all

FROM i386/ubuntu:latest
COPY --from=nwnsc /tmp/nwnsc/nwnsc/nwnsc /usr/local/bin/
COPY --from=nwserver /nwn/data /nwn/data
WORKDIR /tmp
ENTRYPOINT ["nwnsc", "-on", "/nwn/data"]
CMD ["*.nss"]
