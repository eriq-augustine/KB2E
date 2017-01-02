#!/bin/sh

if [ $# -ne 1 ]; then
   echo "USAGE: $0 <nell file>"
   exit 1
fi

NELL_FILE=$1

psql nell < create.sql

ruby parseNell.rb "${NELL_FILE}"

psql nell < insertAll.sql
psql nell < optimize.sql
