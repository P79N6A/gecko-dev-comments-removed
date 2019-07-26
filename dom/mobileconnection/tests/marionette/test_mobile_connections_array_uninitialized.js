


MARIONETTE_TIMEOUT = 1000;

SpecialPowers.addPermission("mobileconnection", true, document);



let ifr = document.createElement("iframe");
let connections;

ifr.onload = function() {
  connections = ifr.contentWindow.navigator.mozMobileConnections;

  
  ok(connections);
  is(connections.length, 1);

  ifr.parentNode.removeChild(ifr);
  ifr = null;
  connections = null;

  SpecialPowers.gc();
  cleanUp();
};
document.body.appendChild(ifr);

function cleanUp() {
  SpecialPowers.removePermission("mobileconnection", document);
  finish();
}
