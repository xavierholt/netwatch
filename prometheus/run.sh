#! /bin/bash
docker run --rm -it -p 9090:9090 -v "$(pwd)/prometheus.yml:/prometheus/prometheus.yml" prom/prometheus --log.level=debug
