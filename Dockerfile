FROM ubuntu:20.04
WORKDIR /app

COPY ./qs_server/db /app/qs_server/db
COPY ./qs_server/include /app/qs_server/include
COPY ./qs_server/src /app/qs_server/src
COPY ./shared /app/shared

EXPOSE 8080

RUN apt-get update && \
    apt-get install -y \
    build-essential \
    libsqlite3-dev \
    libssl-dev

# Delete all old obj files
RUN find ./ -type f -name "*.o" -delete

CMD ["make", "./qs_server/src/"]

# To run docker run -p 127.0.0.1:8080:8080 -it qs-server bash
