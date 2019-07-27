


const protocol = require("devtools/server/protocol");

const HelloActor = protocol.ActorClass({
  typeName: "helloActor",

  hello: protocol.method(function () {
    return;
  }, {
    request: {},
    response: {}
  })
});
