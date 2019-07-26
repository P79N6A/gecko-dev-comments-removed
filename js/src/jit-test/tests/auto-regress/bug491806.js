


uneval(new Function("\
  for(\
    ((let (functional) x) for each ([] in [])); \
    yield x; \
    (let (x = true) x));\
"))
