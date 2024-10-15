#!/usr/bin/bash
docker build -t mmandcc ./
docker run --rm -e PROJECT_PATH=$(pwd) -v $(pwd):$(pwd) -w $(pwd) mmandcc:latest
