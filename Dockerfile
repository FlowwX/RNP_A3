FROM ubuntu

RUN apt-get update
RUN	apt-get install --assume-yes iputils-ping
RUN	apt-get install --assume-yes net-tools
RUN apt-get install --assume-yes gcc
RUN apt-get install --assume-yes make
RUN apt-get install --assume-yes telnet