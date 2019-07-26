


MARIONETTE_TIMEOUT = 60000;

SpecialPowers.addPermission("mobileconnection", true, document);



let ifr = document.createElement("iframe");
let connection;
ifr.onload = function() {
  connection = ifr.contentWindow.navigator.mozMobileConnections[0];

  ok(connection instanceof ifr.contentWindow.MozMobileConnection,
     "connection is instanceof " + connection.constructor);

  nextTest();
};
document.body.appendChild(ifr);

let caseId = 0;
let options = [
  buildOption(5, true, '0000', 0),  

  
  buildOption(null, true, '0000', 0),
  buildOption(0, null, '0000', 0),
  buildOption(0, true, null, 0),
  buildOption(0, true, '0000', null),

  
  {'enabled': true, 'password': '0000', 'serviceClass': 0},
  {'program': 0, 'password': '0000', 'serviceClass': 0},
  {'program': 0, 'enabled': true, 'serviceClass': 0},
  {'program': 0, 'enabled': true, 'password': '0000'},
];

function buildOption(program, enabled, password, serviceClass) {
  return {
    'program': program,
    'enabled': enabled,
    'password': password,
    'serviceClass': serviceClass
  };
}

function testSetCallBarringOptionError(option) {
  let request = connection.setCallBarringOption(option);
  request.onsuccess = function() {
    ok(false,
       'should not fire onsuccess for invaild call barring option: '
       + JSON.stringify(option));
  };
  request.onerror = function(event) {
    is(event.target.error.name, 'InvalidParameter', JSON.stringify(option));
    nextTest();
  };
}

function nextTest() {
  if (caseId >= options.length) {
    cleanUp();
  } else {
    let option = options[caseId++];
    log('test for ' + JSON.stringify(option));
    testSetCallBarringOptionError(option);
  }
}

function cleanUp() {
  SpecialPowers.removePermission("mobileconnection", document);
  finish();
}
