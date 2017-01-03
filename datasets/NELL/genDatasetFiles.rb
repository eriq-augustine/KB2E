#!/bin/ruby

require 'fileutils'

OUT_DIR = 'out'

ENTITY_ID_FILE = File.join(OUT_DIR, 'entity2id.txt')
RELATION_ID_FILE = File.join(OUT_DIR, 'relation2id.txt')
TEST_FILE = File.join(OUT_DIR, 'test.txt')
TRAIN_FILE = File.join(OUT_DIR, 'train.txt')
VAILD_FILE = File.join(OUT_DIR, 'valid.txt')

def main(args)
   FileUtils.mkdir_p(OUT_DIR)

end

if (__FILE__ == $0)
   main(ARGV)
end



