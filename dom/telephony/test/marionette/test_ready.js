


MARIONETTE_TIMEOUT = 60000;

SpecialPowers.addPermission("telephony", true, document);

function cleanUp() {
  SpecialPowers.removePermission("telephony", document);
  finish();
}

let telephony = window.navigator.mozTelephony;
ok(telephony);

telephony.onready = function() {
  log("Receive 'ready' event");

  
  let iframe = document.createElement("iframe");
  iframe.addEventListener("load", function load() {
    iframe.removeEventListener("load", load);

    let iframeTelephony = iframe.contentWindow.navigator.mozTelephony;
    ok(iframeTelephony);

    iframeTelephony.onready = function() {
      log("Receive 'ready' event in iframe");

      cleanUp();
    };
  });

  document.body.appendChild(iframe);
};
