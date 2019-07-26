


MARIONETTE_TIMEOUT = 30000;

SpecialPowers.addPermission("mobilenetwork", true, document);

let connection = navigator.mozMobileConnections[0];
ok(connection instanceof MozMobileConnection,
   "connection is instanceof " + connection.constructor);


function testLastKnownNetwork() {
  log("testLastKnownNetwork: " + connection.lastKnownNetwork);
  
  is(connection.lastKnownNetwork, "310-260");
  runNextTest();
}

function testLastKnownHomeNetwork() {
  log("testLastKnownHomeNetwork: " + connection.lastKnownHomeNetwork);
  
  is(connection.lastKnownHomeNetwork, "310-260");
  runNextTest();
}

let tests = [
  testLastKnownNetwork,
  testLastKnownHomeNetwork
];

function runNextTest() {
  let test = tests.shift();
  if (!test) {
    cleanUp();
    return;
  }

  test();
}

function cleanUp() {
  SpecialPowers.removePermission("mobilenetwork", document);
  finish();
}

runNextTest();
