#!/bin/sh

# Requires:
# - dot (with png support)
# - ImageMagic (or any "display" command alternative)

TEMPDOT=$(mktemp --suffix .dot) &&
time ./bacon "$@" >$TEMPDOT &&
TEMPPNG=$(mktemp --suffix .png) &&
TEMPPNG=test.png &&
dot -Tpng -Gcharset=latin1 $TEMPDOT >$TEMPPNG &&
rm -f $TEMPDOT &&
display $TEMPPNG &&
rm -f $TEMPPNG
