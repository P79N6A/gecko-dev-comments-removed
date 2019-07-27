




const { RootActor } = devtools.require("devtools/server/actors/root");

var gMainConnection, gMainTransport;
var gSubconnection1, gSubconnection2;
var gClient;

function run_test()
{
  DebuggerServer.init();

  add_test(createMainConnection);
  add_test(TestNoForwardingYet);
  add_test(createSubconnection1);
  add_test(TestForwardPrefix1OnlyRoot);
  add_test(createSubconnection2);
  add_test(TestForwardPrefix12OnlyRoot);
  add_test(TestForwardPrefix12WithActor1);
  add_test(TestForwardPrefix12WithActor12);
  run_next_test();
}











function newConnection(aPrefix)
{
  var conn;
  DebuggerServer.createRootActor = function (aConn) {
    conn = aConn;
    return new RootActor(aConn, {});
  };

  var transport = DebuggerServer.connectPipe(aPrefix);

  return { conn: conn, transport: transport };
}


function createMainConnection()
{
  ({ conn: gMainConnection, transport: gMainTransport }) = newConnection();
  gClient = new DebuggerClient(gMainTransport);
  gClient.connect((aType, aTraits) => run_next_test());
}















function tryActors(aReachables, aCompleted) {
  let count = 0;

  let outerActor;
  for (outerActor of [ 'root',
                       'prefix1/root', 'prefix1/actor',
                       'prefix2/root', 'prefix2/actor' ]) {
    



    let actor = outerActor;

    count++;

    gClient.request({ to: actor, type: 'echo', value: 'tango'}, 
                    (aResponse) => {
                      if (aReachables.has(actor))
                        do_check_matches({ from: actor, to: actor, type: 'echo', value: 'tango' }, aResponse);
                      else
                        do_check_matches({ from: actor, error: 'noSuchActor', message: "No such actor for ID: " + actor }, aResponse);

                      if (--count == 0)
                        do_execute_soon(aCompleted, "tryActors callback " + aCompleted.name);
                    });
  }
}






function TestNoForwardingYet()
{
  tryActors(new Set(['root']), run_next_test);
}








function newSubconnection(aPrefix)
{
  let { conn, transport } = newConnection(aPrefix);
  transport.hooks = {
    onPacket: (aPacket) => gMainConnection.send(aPacket),
    onClosed: () => {}
  }
  gMainConnection.setForwarding(aPrefix, transport);

  return { conn: conn, transport: transport };
}


function createSubconnection1()
{
  let { conn, transport } = newSubconnection('prefix1');
  gSubconnection1 = conn;
  transport.ready();
  gClient.expectReply('prefix1/root', (aReply) => run_next_test());
}


function TestForwardPrefix1OnlyRoot()
{
  tryActors(new Set(['root', 'prefix1/root']), run_next_test);
}


function createSubconnection2()
{
  let { conn, transport } = newSubconnection('prefix2');
  gSubconnection2 = conn;
  transport.ready();
  gClient.expectReply('prefix2/root', (aReply) => run_next_test());
}

function TestForwardPrefix12OnlyRoot()
{
  tryActors(new Set(['root', 'prefix1/root', 'prefix2/root']), run_next_test);
}







function EchoActor(aConnection)
{
  this.conn = aConnection;
}
EchoActor.prototype.actorPrefix = "EchoActor";
EchoActor.prototype.onEcho = function (aRequest) {
  



  return JSON.parse(JSON.stringify(aRequest));
};
EchoActor.prototype.requestTypes = {
  "echo": EchoActor.prototype.onEcho
};

function TestForwardPrefix12WithActor1()
{
  let actor = new EchoActor(gSubconnection1)
  actor.actorID = 'prefix1/actor';
  gSubconnection1.addActor(actor);

  tryActors(new Set(['root', 'prefix1/root', 'prefix1/actor', 'prefix2/root']), run_next_test);
}

function TestForwardPrefix12WithActor12()
{
  let actor = new EchoActor(gSubconnection2)
  actor.actorID = 'prefix2/actor';
  gSubconnection2.addActor(actor);

  tryActors(new Set(['root', 'prefix1/root', 'prefix1/actor', 'prefix2/root', 'prefix2/actor']), run_next_test);
}
