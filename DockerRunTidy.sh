#!/usr/bin/bash
docker run -it -e PROJECT_PATH=$(pwd) -v $(pwd):$(pwd) -w $(pwd) mmandcc:latest
