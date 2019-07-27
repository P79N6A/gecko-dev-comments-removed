
var g = newGlobal();
var dbg = new Debugger;
var gw = dbg.addDebuggee(g);

var DOfp = gw.getOwnPropertyDescriptor('Function').value.proto;

print(DOfp.script.source.introductionScript);
