-- FITIn is completely controllable by a script written in Lua
-- (http://lua.org/). 
--
-- This is a sample of a very simple use case with additional
-- documentation for more options.
--
-- In general, FITIn looks for various callbacks and invokes them
-- when applicable.
--
-- tl;wr : Scroll down to `flip_value'.

-- General notes, READ THEM CAREFULLY!
--
-- There is no glibc when compiling code for Valgrind tools. This is why
-- some limitations to the Lua runtime apply:
--
--   * No floating-point types, no trigonometric functions. Only integers.
--   * No locale-aware string handling. getlocale will always return the
--     "C"-locale. Be careful when using non-ASCII strings.
--   * No absolute time. Instead, time.h functions deal with the relative
--     time in ms since starting the program. So be careful when using
--     os.time() for math.randomseed() in case of security-critical
--     things. 
--
-- Debugging notes:
--   * You can use print(). But it is only visible when starting
--     Valgrind with --verbose.
--   * If FITIn complains about an invalid --control-script parameter,
--     try to load it in a stand-alone lua-environment for more
--     comprehensive error messages.
--   * Runtime errors of Lua are visible in stdout, possibly interfering
--     with the running program's output.
--

-- treat_superblock:
--
-- This callback is used once per superblock before instrumentation. If
-- the binary contains debugging symbols (-g/-g3), more options than
-- `address' become non-empty.
--
--   * address:integer: The initial address of the superblock.
--   * fnname:string: Name of the superblock's function.
--   * filename:string: Name of the original source code file.
--   * dirname:string: Location (full path) of the original source code
--     file.
--   * linenum:integer: Line number of the beginning instruction of this
--     superblock.
--
--  Expected return: boolean. True or false for selecting specific
--  super blocks. Rejecting unnecessary ones helps improving the
--  performance.
--
--  If missing: Every super block is monitored.
--
treat_superblock = function(address, fnname, filename, dirname, linenum)
  return fnname == "main"
end

-- monitor_address:
--
-- Whenever memory in an applicable super block is loaded, FITIn checks
-- whether this is a relevant location. In your source code, you may
-- have already annotated variables. These are which we regularly want
-- to monitor.
--
--   * address:integer: The address to load from.
--   * annotated:boolean: Flag, whether FITIn has seen a source code
--     annotation for this location.
--   * size:integer: The size given to this address.
--
-- Expected return: boolean. True or false for selecting specific
-- variables. The ones rejected will not be receive a `flip_value' call.
-- 
-- If missing: FITIn only selects addresses having a code annotation.
-- The following listing is only for demonstration.
--
monitor_address = function(address, annotated, size)
  return annotated
end

-- flip_value:
--
-- Right before using a value from a selected memory address, FITIn will
-- call this method. Using this methods, you can write arrays of
-- bit-patterns that select the bits to flip.
--
--   * state: Context data you cannot read or modify in Lua.
--   * address:integer: The address of the value to flip.
--   * counter:integer: A global counter value for every call on `flip_value'.
--   * size:integer: The size of this value in byte.
--   * desc:string: A Valgrind-formatted string describing the target by
--     information taken from the debug symbols. You need to run
--     `--verbose' or enable debug symbols by `before_start' to get this
--     string. If not, this is nil. Otherwise it can be nil as well.
--
--  Expected return: array. You can use `{}' as an empty array to state
--  "don't touch", e.g. for doing dry-runs. Otherwise, each element of
--  the array must be an integer. In FITIn-Lua, every integer takes
--  64bits, regardless of the platform. In the case that you know that
--  you are dealing with larger data (e.g. SIMD vector types), you can
--  specifiy multiple elements, the first one describing bits 0..63, the
--  second one for 64..127 and so on. It can be less or more, FITIn
--  selects the appropriate size skipping larger arrays and leaving bits
--  untouched that are not reachable by your returned value.
--
--  If missing: No bit-flip will take place.
--
--  NOTE: By default, none of your flips become persistant (except by
--  syscalls operating directly on RAM), being lost after the next
--  reload. To prevent this, you can call `persist_flip' from this
--  function.
--
--  persist_flip
--
--    * state: Please use the `state' variable from the same scope.
--    * bit-patterns: array. An array of the same type as the return
--      value of `flip_value'. It's your choice whether they are the
--      same or not.
--
--  If your array contains more bytes than technically possible for a
--  register-ready value, `persist_flip' will write on later bytes in
--  memory if the registered value is larger. Think of a C-struct.
--
flip_value = function(state, address, counter, size, desc)
  -- on the third global access to selected variables
  if counter == 3 then
    -- bit-flip pattern (LSB->MSB): 000001000...
    -- 
    -- We could make the flip persistant this way:
    -- persist_flip(state, {32})
    return {32}
  else
    return {}
  end
end

-- before_start:
--
-- The callback is called once before starting the program. You can use
-- it for initialization or logging.
--
-- Optional return: integer. The return value may control certain
-- execution options by bits set. Currently available:
--
--   * 2^0: If set, Valgrind will try to read debugging symbols from the
--     program. Regardless of this option, this will also happen when using
--     `--verbose' command line option. While this may lead to a little
--     launch delay, this allows reading target description strings
--     provided by Valgrind when flipping.
--
-- before_start = function()
--   print("Hello!")
--   return 1
-- end

-- after_end:
--
-- The callback is called once after terminating the program. You can use
-- it for clean-up or logging.
--
-- No parameters, no returns, no dependent actions.
--
-- after_end = function()
--   print("Bye!")
-- end

-- next_block:
--
-- This callback is called each time after leaving a super block. It
-- offers some additional ways to control the runtime:
--
--   * instructions:integer: The number of instructions executed by the
--     program and its depending libraries. Valgrind/FITIn-internals don't
--     count.
--
-- Expected return: integer.
-- 
--   * 1: Stop FITIn. FITIn will not continue to ask you about new
--        superblocks (skipping), monitorable variables (skipping) and will 
--        not perform any further flips. This will also stop counting instructions
--        that have not been detected so far. This can help improving the performance.
--   * 2: This will terminate the program and Valgrind-process immediately.
--   * any other: continue (if 1 has never been returned so far).
--
--   If missing: always continues.

-- monitor_field
--
--   Convenience function after Fortran macro FITIN_MONITOR_FIELD(x).
--   This function takes the start address of a field, the total size
--   and the number of elements. This allows some more reliable flagging
--   of elements somewhere in a field w.r.t. dimensions.
--
--     * address:integer: The start address of the field.
--     * size:integer: The overall size of this field in bytes.
--     * dimensions:table: A one-dimensional table containing n elements
--       if the field was specified with n dimensions. Every i of n
--       contains the size of the i-th dimension.
--
--   Note: There is no return value. Use add_address here to flag
--   individual cells for being monitored.

-- breakpoint:
--
-- Called whenever the code hits a FITIN_BREAKPOINTi macro. Can be used to pass
-- data to the script aside from any other callbacks.
--
--   * a,b,c,d,e:integer: arbitrary integer values. 0 if not set.
--
-- No Return.

-- Can be called directly from virtually any callback:
--
--
-- flip_on_memory
--
--   * address:integer: The address where you want the bit flip to take place.
--   * size:integer: The size of the data that you want to flip.
--   * pattern:array. Bit-patterns as if returned by `flip_value'.
--
-- No return value.
--
-- WARNING: Invalid addresses or ranges may cause crashes.
--
--
-- add_address
--
-- Equivalent to FITIN_MONITOR_MEMORY(mem, size).
--
--   * address:integer
--   * size:integer
--
--
-- remove_address
--
-- Equivalent to FITIN_UNMONITOR_MEMORY(mem, size).
--
--   * address:integer
--   * size:integer
--

-- FILE LOCKS
--
-- On Linux, and for multiple or distributed Valgrind processes, you can try to
-- use file locks to achive exclusive operations.
--
-- You need to C-define FITIN_WITH_LUA_LOCKS when compiling FITIn as this feature
-- is not fully platform independent yet (modify `Makefile.am' here and re-run
-- `autogen.sh').
--
-- Once compiled, we enabled some functions from the LuaFileSystem 
-- (http://keplerproject.github.io/luafilesystem/), most notably `lfs.lock' and
-- `lfs.unlock'.
--
-- Put these into one of the callbacks to have one of the processes gain the lock.
--
