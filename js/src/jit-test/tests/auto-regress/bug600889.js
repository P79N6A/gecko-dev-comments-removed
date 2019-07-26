


uneval = function(){}
Function("\
  function zz(aa) {\
    if (aa) this.a = decodeURIComponent;\
    gc();\
    delete this.a\
  }\
  for each(c in [0, 0, 0, 0, 0, 0, 0, new Boolean(false), \
                  0, new Boolean(false), new Boolean(false), \"\"]) {\
    l=new zz(c)\
  }\
")()
