monitor_address = function(address, annotated)
  return annotated
end

treat_superblock = function(address, fnname, filename, dirname, linenum)
  return fnname == "main"
end

flip_value = function(state, address, counter, size)
  if counter == 1 then
    persist_flip(state, {16384})
    return {16384}
  else
    return {}
  end
end
