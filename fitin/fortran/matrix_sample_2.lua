-- This sample demonstrates another possible use of FITIn inside of
-- Fortran matrices.
--
-- Different to the previous sample, we have the regular way leaving
-- many implementations to the compiler.
--
-- Out code is like
--
-- m1 = reshape((/1,2,3,4,5,6,7,8,9/), shape(m1))
-- m2 = reshape((/1,2,3,4,5,6,7,8,9/), shape(m2))
--
--  m3 = matmul(m1, m2)
--  -- CALL FITIN_MONITOR_FIELD(m3)
--  v1 = m3(1,:)
--
-- To dynamically work on `m3` and not missing the slice on `v1`, we
-- make use of FITIN_MONITOR_FIELD(m3). This will call `monitor_field`
-- on `m3` which allows us to have all the freedom we need to select
-- arbitrary fields on `m3` in Lua. 


monitor_field = function(address, size, dimensions)
  elem_size = 0
  elems = 1

  -- Get the total number of elements in this field.
  for k, v in pairs(dimensions) do
    elems = elems * v
  end

  -- Get the size of every single element
  elem_size = size / elems

  -- Activate the 2nd element in this field. By knowing
  -- the total size, we can use add_address on lots of fields
  -- here.
  add_address(address + elem_size, elem_size)
  --add_address(address + elem_size * 2, elem_size)
end

monitor_address = function(address, annotated, size)
  return annotated
end

flip_value = function()
  return {4}
end

-- In this example, we assume everything is inside of MAIN.
treat_superblock = function(address, fnname, filename, dirname, linenum)
  return fnname == "MAIN__"
end
