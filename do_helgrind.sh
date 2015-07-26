#!/bin/bash

/usr/bin/valgrind -v --trace-children=yes --tool=helgrind --read-var-info=yes ./atanks


