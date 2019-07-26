

assert_subprinter_registered('SpiderMonkey', 'ptr-to-JSObject')
run_fragment('JSObject.simple')






assert_pretty('glob', '(JSObject *)  [object global] delegate')
assert_pretty('plain', '(JSObject *)  [object Object]')
assert_pretty('func', '(JSObject *)  [object Function "dys"]')
assert_pretty('anon', '(JSObject *)  [object Function <unnamed>]')
assert_pretty('funcPtr', '(JSFunction *)  [object Function "formFollows"]')
