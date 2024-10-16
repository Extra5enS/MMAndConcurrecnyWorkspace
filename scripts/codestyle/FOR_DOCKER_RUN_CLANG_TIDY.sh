#!/usr/bin/bash
python3 scripts/codestyle/clang_tidy_check.py --full $PROJECT_PATH $PROJECT_PATH/build
