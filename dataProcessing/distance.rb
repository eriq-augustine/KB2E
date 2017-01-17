module Distance
   # This is actual the l1 distance between (head + relation) and tail
   def Distance.l1(head, tail, relation)
      return (tail - (head + relation)).abs()
   end

   # This is actual the l2 distance between (head + relation) and tail
   def Distance.l2(head, tail, relation)
      return (tail - (head + relation)) ** 2
   end
end
