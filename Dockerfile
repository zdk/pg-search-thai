FROM postgres:18

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    libthai-dev \
    postgresql-server-dev-18 \
    && rm -rf /var/lib/apt/lists/*

COPY . /usr/src/pg-search-thai
WORKDIR /usr/src/pg-search-thai

RUN make -C thai_parser LIBDIR=/usr && make -C thai_parser LIBDIR=/usr install
RUN make -C thai_dictionary install

RUN echo "CREATE EXTENSION thai_parser;" > /docker-entrypoint-initdb.d/01-init.sql
