treat_superblock = function(address, fnname, filename, dirname, linenum)
  return fnname == "main"
end

monitor_address = function(address, annotated)
  return annotated
end

flip_pattern = 0

flip_value = function(state, address, counter)
  if counter == 1 then
    return { flip_pattern }
  else
    return {}
  end
end

breakpoint = function(nr, a, b, c, d)
  print("BP!! " .. nr .. ", " .. a .. ", " .. b .. ", " .. c .. ", " .. d)
  if nr == 1 then
    if a == 4 and b == 3 and c == 0 and d == -1 then
      flip_pattern = bit32.bor(flip_pattern, 1)
    end
  elseif nr == 2 then
    if a == 4 and b == 3 and c == 0 then
      flip_pattern = bit32.bor(flip_pattern, 2)
    end
  elseif nr == 3 then
    if a == 4 and b == 3 then
      flip_pattern = bit32.bor(flip_pattern, 4)
    end
  elseif nr == 4 then
    if a == 4 then
      flip_pattern = bit32.bor(flip_pattern, 8)
    end
  elseif nr == 5 then
    flip_pattern = bit32.bor(flip_pattern, 16)
  else
    flip_pattern = bit32.bor(flip_pattern, 32)
  end
end
