

var g = newGlobal('new-compartment');
g.eval("var x = 'some-atom';");

schedulegc(this);
schedulegc('atoms');
gc('compartment');
print(g.x);
