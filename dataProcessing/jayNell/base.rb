module Base
   ENTITY_ID_FILENAME = 'entity2id.txt'
   RELATION_ID_FILENAME = 'relation2id.txt'
   TEST_FILENAME = 'test.txt'
   TRAIN_FILENAME = 'train.txt'
   VALID_FILENAME = 'valid.txt'

   # Make sure to leave out the evaluation gold truth.
   TRIPLE_FILENAMES = [
      'label-train-uniq-raw-rel.db.TRAIN',
      'NELL.08m.165.cesv.csv.CandRel_CBL.out',
      'NELL.08m.165.cesv.csv.CandRel_CPL.out',
      'NELL.08m.165.cesv.csv.CandRel_General.out',
      'NELL.08m.165.cesv.csv.CandRel.out',
      'NELL.08m.165.cesv.csv.CandRel_SEAL.out',
      'NELL.08m.165.cesv.csv.PattRel.out',
      'NELL.08m.165.esv.csv.PromRel_General.out',
      'seed.165.rel.uniq.out',
      'seed.165.rel.uniq_te.out',
      'testTargets.additional.Rel.out',
      'testTargets.additional.ValRel.out',
      'testTargets.shangpu.Rel.out',
      'testTargets.shangpu.ValRel.out',
      'trainTargets.Rel.out',
      'trainTargets.ValRel.out',
      'wlTargets.Rel.out'
   ]

   CATEGORY_FILES = [
      'label-train-uniq-raw-cat.db.TRAIN',
      'NELL.08m.165.cesv.csv.CandCat_CBL.out',
      'NELL.08m.165.cesv.csv.CandCat_CMC.out',
      'NELL.08m.165.cesv.csv.CandCat_CPL.out',
      'NELL.08m.165.cesv.csv.CandCat_General.out',
      'NELL.08m.165.cesv.csv.CandCat_Morph.out',
      'NELL.08m.165.cesv.csv.CandCat.out',
      'NELL.08m.165.cesv.csv.CandCat_SEAL.out',
      'NELL.08m.165.cesv.csv.PattCat.out',
      'NELL.08m.165.esv.csv.PromCat_General.out',
      'seed.165.cat.uniq.out',
      'seed.165.cat.uniq_te.out',
      'testTargets.additional.Cat.out',
      'testTargets.additional.ValCat.out',
      'testTargets.shangpu.Cat.out',
      'trainTargets.Cat.out',
      'wlTargets.Cat.out',
      'testTargets.shangpu.ValCat.out',
      'trainTargets.ValCat.out'
   ]

   def Base.readTriples(path, minConfidence = 0.0)
      triples = []
      rejectedCount = 0

      File.open(path, 'r'){|file|
         file.each{|line|
            parts = line.split("\t").map{|part| part.strip()}
            if (parts[3].to_f() < minConfidence)
               rejectedCount += 1
               next
            end

            triples << parts[0...3].map{|part| part.to_i()}
         }
      }

      return triples, rejectedCount
   end

   def Base.writeEntities(path, triples)
      entities = []
      entities += triples.map{|triple| triple[0]}
      entities += triples.map{|triple| triple[1]}
      entities.uniq!

      File.open(path, 'w'){|file|
         file.puts(entities.map.with_index{|entity, index| "#{entity}\t#{index}"}.join("\n"))
      }
   end

   def Base.writeRelations(path, triples)
      relations = triples.map{|triple| triple[2]}
      relations.uniq!

      File.open(path, 'w'){|file|
         file.puts(relations.map.with_index{|relation, index| "#{relation}\t#{index}"}.join("\n"))
      }
   end

   def Base.writeTriples(path, triples)
      File.open(path, 'w'){|file|
         # Head, Tail, Relation
         file.puts(triples.map{|triple| "#{triple[0]}\t#{triple[1]}\t#{triple[2]}"}.join("\n"))
      }
   end
end
