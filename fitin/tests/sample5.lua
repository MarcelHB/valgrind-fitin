monitor_address = function(address, annotated)
  if annotated then
    return true
  else
    return false
  end
end

treat_superblock = function(address, fnname, filename, dirname, linenum)
  if fnname == "main" then
    return true
  else
    return false
  end
end

flip_value = function(state, address, counter, size)
  if counter == 2 then
    return {1}
  else
    return {}
  end
end
