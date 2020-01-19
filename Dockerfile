FROM beamdog/nwserver:latest as nwserver
LABEL maintainers "jakobknutsen@gmail.com"

FROM phusion/holy-build-box-32:latest as nwnsc
COPY ./ /tmp/nwnsc/
WORKDIR /tmp/nwnsc
RUN yum install bison -y
RUN /hbb_exe/activate-exec bash -x -c 'cmake . && make all'

FROM i386/ubuntu:latest
COPY --from=nwnsc /tmp/nwnsc/nwnsc/ /usr/local/bin/
COPY --from=nwserver /nwn/data /nwn/data
WORKDIR /tmp
ENTRYPOINT ["nwnsc", "-n", "/nwn/data"]
CMD ["*.nss"]
