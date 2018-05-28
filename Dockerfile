FROM ubuntu

RUN apt-get update
RUN	apt-get install --assume-yes iputils-ping
RUN	apt-get install --assume-yes net-tools