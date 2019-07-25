var prox = Proxy.create({
  getPropertyDescriptor: function() { return undefined; },
  has:                   function() { return true; },
});


newGlobal("new-compartment").__lookupSetter__.call(prox, "e");
