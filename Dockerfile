FROM ubuntu:latest

RUN apt-get update && \
    apt-get install -y git build-essential && \
    rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/writemorecode/vigilant-pancake.git && \
    cd vigilant-pancake && \
    make

WORKDIR /vigilant-pancake
EXPOSE 5000
CMD ["./server"]

