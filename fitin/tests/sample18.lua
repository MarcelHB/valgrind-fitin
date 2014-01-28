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

first_address = 0
second_flipped = false

flip_value = function(state, address, counter, size)
	if first_address == 0 then
		first_address = address
	else
		if not second_flipped then
			second_flipped = true
			flip_on_memory(first_address + 1, 1, {1})
		end
	end
	return {}
end
