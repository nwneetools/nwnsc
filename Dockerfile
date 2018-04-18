FROM beamdog/nwserver:latest as nwserver

FROM phusion/holy-build-box-32:latest as nwnsc
LABEL maintainers "jakobknutsen@gmail.com & glorwinger"
WORKDIR /tmp
COPY ./ ./nwnsc/
RUN buildDeps="bison" \
    && yum install $buildDeps -y \
    && cd nwnsc \
    && /hbb_exe/activate-exec bash -x -c 'cmake /tmp/nwnsc' \
    && make \
    && mv nwnsc/nwnsc /usr/local/bin \
    && cd /tmp
COPY --from=nwserver /nwn/data /nwn/data
ENTRYPOINT ["nwnsc", "-n", "/nwn/data"]
CMD ["*.nss"]
