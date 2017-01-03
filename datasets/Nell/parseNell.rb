require 'fileutils'
require 'uri'

SKIP_FIRST_LINE = true

# Batch the inserts to we don't run out of memory.
MAX_INSERTS = 10000

INSERT_DIR = File.join('sql', 'insert')
ENTITIES_FILE = File.join(INSERT_DIR, 'entities.sql')
RELATIONS_FILE = File.join(INSERT_DIR, 'relations.sql')
LITERALS_FILE = File.join(INSERT_DIR, 'literals.sql')
CATEGORIES_FILE = File.join(INSERT_DIR, 'categories.sql')
TRIPLES_FILE = File.join(INSERT_DIR, 'triples.sql')

ENTITIES_TABLE = 'Entities'
RELATIONS_TABLE = 'Relations'
LITERALS_TABLE = 'EntityLiteralStrings'
CATEGORIES_TABLE = 'EntityCategories'
TRIPLES_TABLE = 'Triples'

# Indexes in line.
ENTITY = 0
RELATION = 1
VALUE = 2
ITERATION_OF_PROMOTION = 3
PROBABILITY = 4
SOURCE = 5
ENTITY_LITERALSTRINGS = 6
VALUE_LITERALSTRINGS = 7
BEST_ENTITY_LITERALSTRING = 8
BEST_VALUE_LITERALSTRING = 9
CATEGORIES_FOR_ENTITY = 10
CATEGORIES_FOR_VALUE = 11
CANDIDATE_SOURCE = 12

COL_TYPE_STRING = 'string'
COL_TYPE_INT = 'int'
COL_TYPE_FLOAT = 'float'
COL_TYPE_BOOL = 'bool'

# [{valueIndex => value, ...}, ...] will be returned.
# One hash per line.
def fetchValues(path, indexes)
   values = []
   first = true

   File.open(path, 'r'){|file|
      file.each{|line|
         if (first && SKIP_FIRST_LINE)
            first = false
            next
         end

         # Make sure not to strip first just in case there are empty files at the end.
         parts = line.split("\t").map{|val| val.strip()}

         values << indexes.map{|valueIndex| [valueIndex, parts[valueIndex]]}.to_h()
      }
   }

   return values
end

# TODO(eriq)
def escape(string)
   return string.gsub("'", "''")
end

def prepValue(value, options = {})
   options = options || {}

   if (options[:urldecode] == true)
      value = URI.unescape(value)
   end

   if (options[:type] == COL_TYPE_INT)
      if (value == 'NaN')
         value = 'NULL'
      else
         value = "#{value.to_i()}"
      end
   elsif (options[:type] == COL_TYPE_FLOAT)
      if (value == 'NaN')
         value = 'NULL'
      else
         value = "#{value.to_f()}"
      end
   elsif (options[:type] == COL_TYPE_BOOL)
      if (value || value == 'true')
         value = true
      else
         value = false
      end
   else
      value = "'#{escape(value)}'"
   end

   return value
end

# |rowInfo| = [{:name => someName, :options => {...}}, ...]
#  :options is passed blindly to prepValue().
def writeInsert(path, tableName, rows, rowInfo)
   header = ''
   header += "INSERT INTO #{tableName}\n"
   header += "   (#{rowInfo.map{|row| row[:name]}.join(", ")})\n"
   header += 'VALUES'

   File.open(path, 'w'){|file|
      rows.each_slice(MAX_INSERTS){|insertRows|
         valuesString = insertRows.map{|row| "   (#{row.map.with_index{|val, i| prepValue(val, rowInfo[i][:options])}.join(", ")})"}.join(",\n")

         file.puts(header)
         file.puts(valuesString)
         file.puts(";\n")
      }
   }
end

def parseTriples(path)
   columns = [ENTITY, RELATION, VALUE, ITERATION_OF_PROMOTION, PROBABILITY, SOURCE, CANDIDATE_SOURCE]
   rows = fetchValues(path, columns).map{|val| columns.map{|key| val[key]}}

   # Costly, but necessary...
   rows.uniq!

   writeInsert(TRIPLES_FILE, TRIPLES_TABLE, rows, [
      {:name => 'headNellId'},
      {:name => 'relationNellId'},
      {:name => 'tailNellId'},
      {:name => 'promotionIteration', :options => {:type => COL_TYPE_INT}},
      {:name => 'probability', :options => {:type => COL_TYPE_FLOAT}},
      {:name => 'source', :options => {:urldecode => true}},
      {:name => 'candidateSource', :options => {:urldecode => true}}
   ])
end

def parseRelations(path)
   rows = fetchValues(path, [RELATION]).map{|val| [val[RELATION]]}

   # Costly, but necessary...
   rows.uniq!

   writeInsert(RELATIONS_FILE, RELATIONS_TABLE, rows, [{:name => 'nellId'}])
end

def parseEntities(path)
   rawRows = fetchValues(path, [ENTITY, VALUE])
   
   rows  = rawRows.map{|rawRow| [rawRow[ENTITY], true]}
   rows += rawRows.map{|rawRow| [rawRow[VALUE], rawRow[VALUE].start_with?('concept:')]}

   rawRows = nil

   # Costly, but necessary...
   rows.uniq!

   writeInsert(ENTITIES_FILE, ENTITIES_TABLE, rows, [
      {:name => 'nellId'},
      {:name => 'isConcept', :options => {:type => COL_TYPE_BOOL}}
   ])
end

def parseLiterals(path)
   rawRows = fetchValues(path, [
      ENTITY, ENTITY_LITERALSTRINGS, BEST_ENTITY_LITERALSTRING,
      VALUE, VALUE_LITERALSTRINGS, BEST_VALUE_LITERALSTRING
   ])

   rows  = rawRows.map{|rawRow| [rawRow[ENTITY], rawRow[ENTITY_LITERALSTRINGS], rawRow[BEST_ENTITY_LITERALSTRING]]}
   rows += rawRows.map{|rawRow| [rawRow[VALUE], rawRow[VALUE_LITERALSTRINGS], rawRow[BEST_VALUE_LITERALSTRING]]}

   # Clean it up plz.
   rawRows = nil

   # Remove empty values.
   rows.reject!{|row| row[1] == '' && row[2] == ''}

   # Costly, but necessary...
   rows.uniq!

   writeInsert(LITERALS_FILE, LITERALS_TABLE, rows, [{:name => 'entityNellId'}, {:name => 'literal'}, {:name => 'bestLiteral'}])
end

def parseCategories(path)
   rawRows = fetchValues(path, [ENTITY, CATEGORIES_FOR_ENTITY, VALUE, CATEGORIES_FOR_VALUE])

   rows  = rawRows.map{|rawRow| [rawRow[ENTITY], rawRow[CATEGORIES_FOR_ENTITY]]}
   rows += rawRows.map{|rawRow| [rawRow[VALUE], rawRow[CATEGORIES_FOR_VALUE]]}

   # Clean it up plz.
   rawRows = nil

   # Remove empty values.
   rows.reject!{|row| row[1] == ''}

   # Costly, but necessary...
   rows.uniq!

   writeInsert(CATEGORIES_FILE, CATEGORIES_TABLE, rows, [{:name => 'entityNellId'}, {:name => 'category'}])
end

def parseFile(path)
   FileUtils.mkdir_p(INSERT_DIR)

   # To save memory, we are only going to fetch specific values from the file at a time.
   parseEntities(path)
   parseLiterals(path)
   parseCategories(path)
   parseRelations(path)
   parseTriples(path)
end

def main(args)
   if (args.size() != 1)
      puts "USAGE: ruby #{$0} <nell esv.csv file>"
      exit(1)
   end

   parseFile(args[0])
end

if (__FILE__ == $0)
   main(ARGV)
end
