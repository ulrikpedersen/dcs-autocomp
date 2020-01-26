#   docker build -t dcs-autocomp .
#   docker run --rm -it dcs-autocomp
FROM centos:7

RUN yum -y update && yum -y clean all &&\
    yum install -y gcc gcc-c++ make boost-test epel-release &&\
    yum install -y cmake3 &&\
    yum -y clean all && rm -rf /var/cache/yum &&\
    curl -sSLO https://github.com/bats-core/bats-core/archive/v1.1.0.tar.gz &&\
    tar -zxvf v1.1.0.tar.gz &&\
    bats-core-1.1.0/install.sh /usr/local

# Copy the host . dir into src/swmr-testapp
ADD . /src/
WORKDIR /src/cmake-build

RUN cmake3 -DCMAKE_BUILD_TYPE=Debug /src/ &&\
    make VERBOSE=1 && make install &&\
    dcs_compgen -h

