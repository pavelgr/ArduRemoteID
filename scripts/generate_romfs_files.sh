#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd RemoteIDModule

${SCRIPT_DIR}/make_romfs.py romfs_files.h \
                            web/*.html \
                            web/js/*.js \
                            web/styles/*css \
                            web/images/*.jpg \
                            web/images/*.png \
                            public_keys/*.dat

cd -
