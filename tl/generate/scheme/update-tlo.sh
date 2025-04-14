#!/bin/sh
cd $(dirname $0)
tl-parser -e ion_api.tlo ion_api.tl
tl-parser -e tonlib_api.tlo tonlib_api.tl
tl-parser -e lite_api.tlo lite_api.tl
