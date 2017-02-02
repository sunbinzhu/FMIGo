# Build and store this as umit/ubuntu:1 on each build machine, like so:
#
#  docker build -t umit/ubuntu:1 .
#
FROM ubuntu:xenial
RUN apt-get update
#
# Some notes on the packages installed here where it may not be obvious why
# they are installed:
#
#  build-essential: for gcc
#              git: for being able to fetch submodules, since gitlab doesn't
#                   do this for us yet
#           psmisc: for killall
#
RUN apt-get install --yes \
        build-essential cmake ninja-build git python bc psmisc \
        protobuf-compiler protobuf-c-compiler libprotobuf-dev \
        libsuitesparse-dev libzmqpp-dev libhdf5-dev \
        libopenmpi-dev libgsl-dev

#
# Create user, since mpiexec doesn't like to run as root
#
RUN useradd -ms /bin/bash gitlab-ci

# Copy code into image
RUN  mkdir              /home/gitlab-ci/umit
COPY .                  /home/gitlab-ci/umit
RUN  chown -R gitlab-ci /home/gitlab-ci/umit

# Finally, make us run as gitlab-ci
USER gitlab-ci
WORKDIR /home/gitlab-ci/umit

# docker doesn't copy empty directories for some reason
RUN mkdir FMILibrary-2.0.1/ThirdParty/Expat/expat-2.1.0/src
RUN mkdir build
WORKDIR /home/gitlab-ci/umit/build
RUN cmake .. -G Ninja
RUN ninja install
