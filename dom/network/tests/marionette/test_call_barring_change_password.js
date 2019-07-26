


MARIONETTE_TIMEOUT = 60000;

SpecialPowers.addPermission("mobileconnection", true, document);



let ifr = document.createElement("iframe");
let connection;
ifr.onload = function() {
  connection = ifr.contentWindow.navigator.mozMobileConnections[0];

  ok(connection instanceof ifr.contentWindow.MozMobileConnection,
     "connection is instanceof " + connection.constructor);

  setTimeout(testChangeCallBarringPasswordWithFailure, 0);
};
document.body.appendChild(ifr);

function testChangeCallBarringPasswordWithFailure() {
  
  let options = [
    {pin: null, newPin: '0000'},
    {pin: '0000', newPin: null},
    {pin: null, newPin: null},
    {pin: '000', newPin: '0000'},
    {pin: '00000', newPin: '1111'},
    {pin: 'abcd', newPin: 'efgh'},
  ];

  function do_test() {
    for (let i = 0; i < options.length; i++) {
      let request = connection.changeCallBarringPassword(options[i]);

      request.onsuccess = function() {
        ok(false, 'Unexpected result.');
        setTimeout(cleanUp , 0);
      };

      request.onerror = function() {
        ok(request.error.name === 'InvalidPassword', 'InvalidPassword');
        if (i >= options.length) {
          setTimeout(testChangeCallBarringPasswordWithSuccess, 0);
        }
      };
    }
  }

  do_test();
}

function testChangeCallBarringPasswordWithSuccess() {
  
  
  setTimeout(cleanUp , 0);
}

function cleanUp() {
  SpecialPowers.removePermission("mobileconnection", document);
  finish();
}
