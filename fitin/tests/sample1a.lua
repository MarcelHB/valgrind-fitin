monitor_address = function(address, annotated)
  return true
end

treat_superblock = function(address, fnname, filename, dirname, linenum)
  if fnname == "main" then
    return true
  else
    return false
  end
end

flip_value = function(state, address, counter, size)
  if counter == 4 then
    return {8}
  else
    return {}
  end
end
