#!/bin/sh

if [ $# -ne 1 ]; then
   echo "USAGE: $0 <nell file>"
   exit 1
fi

NELL_FILE=$1

psql nell < sql/create.sql

ruby parseNell.rb "${NELL_FILE}"

psql nell < sql/insertAll.sql
psql nell < sql/optimize.sql
