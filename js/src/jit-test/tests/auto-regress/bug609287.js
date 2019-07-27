




f = eval("\
  (function() {\
    __proto__ = \
    Proxy.createFunction((\
    function() {\
      return {\
        has: new ArrayBuffer,\
      }\
    })\
    (\"\"), \
    JSON.parse\
    )\
  })\
")()
