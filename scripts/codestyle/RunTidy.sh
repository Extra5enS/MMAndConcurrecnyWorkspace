#!/usr/bin/bash
docker run --rm -e PROJECT_PATH=$(pwd) -v $(pwd):$(pwd) -w $(pwd) mmandcc:latest
