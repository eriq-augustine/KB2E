module TransE
   def TransE.tripleEnergy(distanceMetric, headVector, tailVector, relationVector)
      sum = 0
      headVector.each_index{|i|
         sum += distanceMetric.call(headVector[i], tailVector[i], relationVector[i])
      }

      return sum
   end
end
