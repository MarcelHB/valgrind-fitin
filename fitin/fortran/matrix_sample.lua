-- This sample demonstrates a possible use of FITIn inside of
-- Fortran matrices.
--
-- Unless you have static dimensions and want to hardcode an
-- address (such as CALL FITIN_MONITOR_VARIABLE(A[X,Y])), you
-- may want to have a more dynamic approach to flip different
-- bits.
--
-- Let's consider a simple matrix multiplication such as
--
-- do i = 1, l
--   do j = 1, m
--     C(i,j) = 0
--     do k = 1, n
--       C(i,j) = C(i,j) + A(k,i) * B(j,k)
--     end do
--   end do
-- end do
--
-- To minimize the changes to the code, we pick one matrix (A)
-- and decide in each iteration what to do. In the FITIn-annotated
-- way, the code now looks like this:
--
-- do i = 1, l
--   do j = 1, m
--     C(i,j) = 0
--     do k = 1, n
--       call FITIN_MONITOR_VARIABLE(A(k,i))
--       C(i,j) = C(i,j) + A(k,i) * B(j,k)
--       call FITIN_UNMONITOR_VARIABLE(A(k,i))
--     end do
--   end do
-- end do
--
-- In this example, we do not know the exact dimensions, but we take
-- the third hit of the FITIn call. As, by the dimensions, we may
-- have multiple accesses to one location, we also have to make sure
-- to limit the flip to a single strike.

-- Address of the third hit
monitored_address = nil
-- To count up to 3.
ref_counter = 0

monitor_address = function(address, annotated)
  -- We only treat annotated variables.
  if annotated then
    ref_counter = ref_counter + 1
    -- The variable that comes third or anything at the same place.
    if ref_counter == 3 or monitored_address == address then
      -- Save the address for future runs of this block.
      monitored_address = address
      return true
    end
  end

  -- Reject anyhting else.
  return false
end

-- In this example, we assume everything is inside of MAIN.
treat_superblock = function(address, fnname, filename, dirname, linenum)
  return fnname == "MAIN__"
end

-- Marker to prevent multiple flips.
flip_counter = 0

flip_value = function(state, address, counter, size)
  -- We only want that single variable, flipped once.
  if flip_counter == 0 and address == monitored_address then
    -- Flip pattern ...010000
    -- Optional, write-back to memory:
    -- persist_flip(state, {16})
    flip_counter = 1
    return {16}
  else
    return {}
  end
end

-- Lets assume A and B are equivalent:
--   1 2 3
--   4 5 6
--   7 8 9
--
-- Under normal circumstances, C will be:
--
--   30  36  42
--   66  81  96
--  102 126 150
--
--  With one flip to an intermediate read of the third A(k,i) by
--  the bit pattern of 16, C is:
--
--   78  36  42
--   66  81  96
--  102 126 150
--
--  Using Lua like this, one can think of many more flip scenarios
--  and targets.
