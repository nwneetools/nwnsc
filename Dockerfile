FROM beamdog/nwserver:latest as nwserver
LABEL maintainers "jakobknutsen@gmail.com"

FROM phusion/holy-build-box-32:latest as nwnsc
COPY ./ /tmp/nwnsc/
WORKDIR /tmp/nwnsc
# Do what we can to update package checksums, because the parent image tends to be outdated
RUN yum -y install yum-plugin-ovl
RUN yum clean all && yum -y update
RUN yum -y install python-hashlib
# Install the actual build deps
RUN yum install -y bison
RUN /hbb_exe/activate-exec bash -x -c 'cmake . && make all'

FROM i386/ubuntu:latest
COPY --from=nwnsc /tmp/nwnsc/nwnsc/ /usr/local/bin/
COPY --from=nwserver /nwn/data /nwn/data
WORKDIR /tmp
ENTRYPOINT ["nwnsc", "-on", "/nwn/data"]
CMD ["*.nss"]
