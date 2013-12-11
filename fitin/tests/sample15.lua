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

flip_value = function(state, address, counter)
  if counter == 2 then
    persist_flip(state, {2 ^ 31})
    return {2 ^ 31}
  else
    return {}
  end
end
