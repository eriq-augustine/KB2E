require './distance'

module TransE
   L1_DISTANCE_STRING = 'L1'
   L2_DISTANCE_STRING = 'L2'

   MAX_ENERGY_THRESHOLD_L1 = 10.0
   MAX_ENERGY_THRESHOLD_L2 = 1.0

   # Remember, each param is an embedding vector.
   def TransE.tripleEnergy(distanceType, head, tail, relation)
      energy = 0

      head.each_index{|i|
         if (distanceType == L1_DISTANCE_STRING)
            energy += Distance.l1(head[i], tail[i], relation[i])
         elsif (distanceType == L2_DISTANCE_STRING)
            energy += Distance.l2(head[i], tail[i], relation[i])
         else
            raise("Unknown distance type: [#{distanceType}]")
         end
      }

      ok = true
      if (distanceType == L1_DISTANCE_STRING && energy > MAX_ENERGY_THRESHOLD_L1)
         ok = false
      elsif (distanceType == L2_DISTANCE_STRING && energy > MAX_ENERGY_THRESHOLD_L2)
         ok = false
      end

      return ok, energy
   end
end
