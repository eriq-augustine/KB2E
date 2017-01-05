#!/bin/sh

if [ $# -ne 1 ]; then
   echo "USAGE: $0 <nell file>"
   exit 1
fi

NELL_FILE=$1

echo "Dropping old tables ..."
psql nell < sql/drop.sql

echo "Creating Tables ..."
psql nell < sql/create.sql

echo "Building Inserts ..."
ruby parseNell.rb "${NELL_FILE}"

echo "Inserting ..."
psql nell <  sql/insert/entities.sql
psql nell <  sql/insert/literals.sql
psql nell <  sql/insert/categories.sql
psql nell <  sql/insert/relations.sql
psql nell <  sql/insert/triples.sql

echo "Optimizing ..."
psql nell < sql/optimize.sql
