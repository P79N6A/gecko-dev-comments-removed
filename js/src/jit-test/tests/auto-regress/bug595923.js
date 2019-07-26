


x = Proxy.createFunction((function () {}), Uint16Array, wrap)
try { new(wrap(x)) } catch(exc1) {}
