
x = (new Debugger).addDebuggee(newGlobal());
print(x.getOwnPropertyDescriptor('Function').value.proto.script);

(new Debugger).memory.takeCensus();
