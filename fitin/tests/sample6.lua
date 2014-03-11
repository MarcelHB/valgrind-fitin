monitor_address = function(address, annotated)
  return annotated
end

treat_superblock = function(address, fnname, filename, dirname, linenum)
  return fnname == "just_do_it"
end

flip_value = function(state, address, counter, size)
  if counter == 10 then
    return {2}
  else
    return {}
  end
end
