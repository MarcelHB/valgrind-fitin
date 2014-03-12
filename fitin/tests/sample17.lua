monitor_address = function(address, annotated)
  return annotated
end

treat_superblock = function(address, fnname, filename, dirname, linenum)
  return fnname == "main"
end

flip_value = function(state, address, counter, size)
  if counter == 1 then
    return {2 ^ 31}
  else
    return {}
  end
end
