





function test() {
  testCreateCommands();
  testRemoveCommands();
}

let tselarr = {
  name: 'tselarr',
  params: [
    { name: 'num', type: { name: 'selection', data: [ '1', '2', '3' ] } },
    { name: 'arr', type: { name: 'array', subtype: 'string' } },
  ],
  exec: function(args, env) {
    return "flu " + args.num + "-" + args.arr.join("_");
  }
};

function testCreateCommands() {
  let gcliIndex = require("gcli/index");
  gcliIndex.addCommand(tselarr);

  let canon = require("gcli/canon");
  let tselcmd = canon.getCommand("tselarr");
  ok(tselcmd != null, "tselarr exists in the canon");
  ok(tselcmd instanceof canon.Command, "canon storing commands");
}

function testRemoveCommands() {
  let gcliIndex = require("gcli/index");
  gcliIndex.removeCommand(tselarr);

  let canon = require("gcli/canon");
  let tselcmd = canon.getCommand("tselarr");
  ok(tselcmd == null, "tselcmd removed from the canon");
}
