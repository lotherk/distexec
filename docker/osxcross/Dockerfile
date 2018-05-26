FROM andrewd/osxcross
MAINTAINER Konrad Lother <k@hiddenbox.org>

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update
RUN apt-get install -y cmake gengetopt


ENV MACOSX_DEPLOYMENT_TARGET=10.8
RUN mkdir -p /opt/osxcross/target/macports
RUN echo "http://nue.de.packages.macports.org/macports/packages" > /opt/osxcross/target/macports/MIRROR
RUN osxcross-macports install pcre json-c libssh2 curl

RUN git clone https://git.hiddenbox.org/lotherk/c-distexec.git /opt/distexec

ADD build.sh /build_distexec.sh

CMD /build_distexec.sh



