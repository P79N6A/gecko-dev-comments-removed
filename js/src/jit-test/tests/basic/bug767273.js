var prox = Proxy.create({
  getOwnPropertyDescriptor: function() { return undefined; },
  has:                      function() { return true; },
});


newGlobal().__lookupSetter__.call(prox, "e");
