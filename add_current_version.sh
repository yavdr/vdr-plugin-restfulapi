#!/bin/bash
VERSION=`git show | head -1 | cut -d ' ' -f 2`

if [ "$VERSION" == '' ]
then
   echo "Git log not found. Will not add a new version"
   exit 1
else
   SHORTVERSION=`echo ${VERSION:0:8}`
   FULLVERSION="0.0.1-git$SHORTVERSION~0yavdr0"
   echo "Compiling version: $FULLVERSION"
   dch -v $FULLVERSION -u medium -m -b 'git snapshot'
fi

