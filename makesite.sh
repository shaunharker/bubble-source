#!/bin/bash
absolute() { echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"; }

CURRENTDIR=`pwd`
PROJECT=$1
AUTHOR=$2
DATE=$3
SRCDIR=$(absolute $4)
ONLINESRCDIR=$5
EXCLUDE=$6

## Create the doxyfile
rm -rf site
mkdir site
cd site
mkdir doxy
cd doxy
doxygen -g > /dev/null

## Fix the doxyfile

# PROJECT_NAME           = "My Project"
# EXTRACT_ALL            = NO
# INPUT                  =
# RECURSIVE              = NO
# GENERATE_XML           = NO
# GENERATE_LATEX         = YES

sed -i '' "s|My Project|$PROJECT|g" Doxyfile
sed -i '' "s|EXTRACT_ALL .*= NO|EXTRACT_ALL = YES|g" Doxyfile
sed -i '' "s|INPUT .*=|INPUT = $SRCDIR|g" Doxyfile
sed -i '' "s|RECURSIVE .*= NO|RECURSIVE = YES|g" Doxyfile
sed -i '' "s|GENERATE_XML .*= NO|GENERATE_XML = YES|g" Doxyfile
sed -i '' "s|EXCLUDE_PATTERNS.*=|EXCLUDE_PATTERNS = $EXCLUDE|g" Doxyfile
sed -i '' "s|GENERATE_LATEX.*= YES|GENERATE_LATEX = NO|g" Doxyfile


## Create the doxygen documentation
doxygen > /dev/null

## Run the XML parser
cd ..
find . | grep "class.*\.xml" | xargs ../bin/doxygen_xml_to_json create > ./programdata.json
sed -i '' "s|$SRCDIR|$ONLINESRCDIR|g" ./programdata.json
## Create index.html
cp ../source/template.html ./index.html
sed -i '' "s|PROJECT|$PROJECT|g" ./index.html
sed -i '' "s|AUTHOR|$AUTHOR|g" ./index.html
sed -i '' "s|DATE|$DATE|g" ./index.html
rm -rf doxy
