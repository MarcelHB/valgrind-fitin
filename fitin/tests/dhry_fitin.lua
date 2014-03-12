monitor_address = function(address, annotated)
  if annotated then
    return true
  else
    return false
  end
end

treat_superblock = function(address, fnname, filename, dirname, linenum)
  if dirname ~= "" then
    reverse_path = string.reverse(dirname)
    idx = string.find(reverse_path, "/")
    reverse_name = string.sub(reverse_path, 0, idx - 1)

    if reverse_name == "nitif_yrhd" then
      return true
    else
      return false
    end
  end
end

flip_value = function(state, address, counter, size)
  if counter == 6 then
    return {16}
  else
    return {}
  end
end
