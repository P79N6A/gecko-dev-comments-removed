

var g = newGlobal();
g.eval("var x = 'some-atom';");

schedulegc(this);
schedulegc('atoms');
gc('compartment');
print(g.x);
