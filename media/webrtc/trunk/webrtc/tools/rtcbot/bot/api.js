












var WebSocketStream = require('websocket-stream');
var Dnode = require('dnode');

function connectToServer(api) {
  var stream = new WebSocketStream("wss://localhost:8080/");
  var dnode = new Dnode(api);
  dnode.on('error', function (error) { console.log(error); });
  dnode.pipe(stream).pipe(dnode);
}





function expose(obj, src, method, casts) {
  obj[method] = function () {
    for (index in casts)
      arguments[index] = new (casts[index])(arguments[index]);
    src[method].apply(src, arguments);
  }
}

window.expose = expose;
window.connectToServer = connectToServer;
