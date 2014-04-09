monitor_address = function(address, annotated)
  return annotated
end

treat_superblock = function(address, fnname, filename, dirname, linenum)
  return fnname == "MAIN__"
end

flip_value = function(state, address, counter, size)
  if counter == 19 then
    return {1}
  else
    return {}
  end
end
  
