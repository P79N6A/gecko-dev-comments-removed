

setJitCompilerOption("baseline.usecount.trigger", 1);
setJitCompilerOption("ion.usecount.trigger", 2);




var x = 0;
function interrupt_gc() {
  if (x++ >= 20)
    return;
  timeout(0.1, interrupt_gc);
  while(x < 20)
    gc();
}

interrupt_gc();
