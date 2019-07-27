



if (helperThreadCount() === 0) {
  quit(0);
}

const root = newGlobal();
root.eval("this.dbg = new Debugger()");
root.dbg.addDebuggee(this);
root.dbg.memory.trackingAllocationSites = true;

offThreadCompileScript(
  "function foo() {\n" +
  "  print('hello world');\n" +
  "}"
);
