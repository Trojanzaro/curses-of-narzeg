FROM debian:12

COPY . /src
WORKDIR /src

RUN \ 
  apt-get update -y \
  && apt-get install -y gcc libncurses5-dev libncursesw5-dev \
  && gcc main.c -o /curses-of-narzeg -lncurses

USER 10000

ENTRYPOINT [ "/curses-of-narzeg" ]

