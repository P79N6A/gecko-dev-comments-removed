





eval("\
  (function(){for(d in[0,Number]) {\
    __defineGetter__(\"\",function(){}),\
    [(__defineGetter__(\"x\",Math.pow))]\
  }})\
")()
delete gc
eval("\
  (function() {\
    for(e in __defineSetter__(\"x\",function(){})){}\
  })\
")()
delete gc

reportCompare(true, true, "don't crash");
