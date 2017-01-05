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
echo "   Entities ..."
psql nell <  sql/insert/entities.sql
echo "   Relations ..."
psql nell <  sql/insert/relations.sql
echo "   Triples ..."
psql nell <  sql/insert/triples.sql

# psql nell <  sql/insert/literals.sql
# psql nell <  sql/insert/categories.sql

echo "Optimizing ..."
psql nell < sql/optimize.sql
