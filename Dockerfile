FROM beamdog/nwserver:latest as nwserver
LABEL maintainers "jakobknutsen@gmail.com"

FROM phusion/holy-build-box-32:latest as nwnsc
COPY ./ /tmp/nwnsc/
WORKDIR /tmp/nwnsc
RUN yum install bison -y
RUN /hbb_exe/activate-exec bash -x -c 'cmake . && make all'
COPY --from=nwserver /nwn/data /nwn/data
ENTRYPOINT ["nwnsc", "-n", "/nwn/data"]
CMD ["*.nss"]
