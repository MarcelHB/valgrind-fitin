monitor_address = function(address, annotated)
  return true
end

treat_superblock = function(address, fnname, filename, dirname, linenum)
  return fnname == "main"
end

flip_value = function(state, address, counter, size)
  if counter == 4 then
    return {1}
  else
    return {}
  end
end
