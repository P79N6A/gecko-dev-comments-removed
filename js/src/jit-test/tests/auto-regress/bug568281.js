




__defineSetter__("x", Array.reduce)
x = Proxy.create(function() {},
this.watch("x",
function() {
  yield
}))
