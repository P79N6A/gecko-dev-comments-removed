var prox = Proxy.create({
  getPropertyDescriptor: function() { return undefined; },
  has:                   function() { return true; },
});


newGlobal().__lookupSetter__.call(prox, "e");
