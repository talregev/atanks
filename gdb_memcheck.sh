#!/bin/bash

/usr/bin/valgrind -v --trace-children=yes --tool=memcheck \
--track-origins=yes --leak-check=full --show-reachable=no \
--read-var-info=yes --vgdb=full --vgdb-error=0 ./atanks


