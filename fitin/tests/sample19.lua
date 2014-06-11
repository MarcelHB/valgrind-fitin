-- Basically, we want to test whether math.random gives us numbers
-- from [3,6] exclusively and counts them by buckets. So at the end,
-- if everything works as expected, we flip the bit so we can see
-- it externally on the code of sample1.c

buckets = {}

before_start = function() 
  for i=0,9 do
    buckets[i] = 0
  end

  math.randomseed(1234)

  for i=0,99 do
    idx = math.random(3,6)
    buckets[idx] = buckets[idx] + 1
  end
end

monitor_address = function(address, annotated)
  return annotated
end

treat_superblock = function(address, fnname, filename, dirname, linenum)
  return fnname == "main"
end

flip_value = function(state, address, counter, size)
  hypothesis = (buckets[0] + buckets[1] + buckets[2] + buckets[7] +
               buckets[8] + buckets[9]) == 0 and
               buckets[3] > 0 and
               buckets[4] > 0 and
               buckets[5] > 0 and
               buckets[6] > 0

  if hypothesis then
    return {32}
  else
    return {}
  end
end
