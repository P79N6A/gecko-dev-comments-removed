



try {
  var x = Proxy.create( {get:function(r,name){return {}[name]}} );
  x.watch('e', function(){});
} catch (exc) {
}

reportCompare(0, 0, 'ok');
