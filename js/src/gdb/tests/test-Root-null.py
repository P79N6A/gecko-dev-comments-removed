














gdb.execute('set print address on')

run_fragment('Root.null')

assert_pretty('null', '0x0')
