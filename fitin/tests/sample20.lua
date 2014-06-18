snd = false

-- Add a[1] lazily after a[0].
monitor_address = function(address, annotated, size)
  -- As a[1] will might a call here on its own, being annotated (Note:
  -- add_address is equiv. to FITIN_MONITOR_ADDRESS), prevent
  -- add_address on &a[1] + size (= b).
  if annotated and not snd then
    add_address(address + size, size)
    snd = true
  end
  return annotated
end

flip_value = function(state, address, counter, size)
  -- Remove a[0] to not being flipped in 2nd use.
  remove_address(address, size)
  -- Persist a[1]
  persist_flip(state, {1})
  return {1}
end

treat_superblock = function(address, fnname, filename, dirname, linenum)
  return fnname == "main"
end
