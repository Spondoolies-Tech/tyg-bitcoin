#!/bin/sh

# This script is a target activated one (pushed ot miner and act there)
# it simple modify any one of the fields in the form of  FIELD_NAME:VALUE 
# where values are numerical
# it's important because changes are sometimes anted to be edited singulariy and there is no good template file
# moreover, sed MUST be used vey carefully, as we cannot edit inplace.
# usng this script will reduce handling errors


FIELD=$1
VALUE=$2

cat /etc/mg_custom_mode |  sed 's/'${FIELD}':[0-9]*/'${FIELD}':'${VALUE}'/g' > /tmp/mgc
cat /tmp/mgc > /etc/mg_custom_mode
rm -f /tmp/mgc
