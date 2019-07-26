


MARIONETTE_TIMEOUT = 10000;

SpecialPowers.addPermission("fmradio", true, document);

let FMRadio = window.navigator.mozFMRadio;

function verifyInitialState() {
  log("Verifying initial state.");
  ok(FMRadio);
  is(FMRadio.enabled, false);

  log("Verifying attributes when disabled.");
  is(FMRadio.frequency, null);
  ok(FMRadio.frequencyLowerBound);
  ok(FMRadio.frequencyUpperBound);
  ok(FMRadio.frequencyUpperBound > FMRadio.frequencyLowerBound);
  ok(FMRadio.channelWidth);

  enableFMRadio();
}

function enableFMRadio() {
  log("Verifying behaviors when enabled.");
  var frequency = FMRadio.frequencyLowerBound + FMRadio.channelWidth;
  var request = FMRadio.enable(frequency);
  ok(request, "FMRadio.enable(r" + frequency + ") returns request");

  request.onsuccess = function() {
    ok(FMRadio.enabled);
    ok(typeof FMRadio.frequency == "number");
    ok(FMRadio.frequency > FMRadio.frequencyLowerBound);
  };

  request.onerror = function() {
    ok(null, "Failed to enable");
  };

  var enabled = false;
  FMRadio.onenabled = function() {
    FMRadio.onenabled = null;
    enabled = FMRadio.enabled;
  };

  FMRadio.onfrequencychange = function() {
    log("Check if 'onfrequencychange' event is fired after the 'enabled' event");
    FMRadio.onfrequencychange = null;
    ok(enabled, "FMRadio is enabled when handling `onfrequencychange`");
    disableFMRadio();
  };
}

function disableFMRadio() {
  log("Verify behaviors when disabled");

  
  
  
  
  var seekRequest = FMRadio.seekUp();
  var seekCompletes = false;
  var failedToSeek = false;
  seekRequest.onerror = function() {
    ok(!seekCompletes);
    failedToSeek = true;
  };

  seekRequest.onsuccess = function() {
    ok(!failedToSeek);
    seekCompletes = true;
  };

  FMRadio.disable();
  FMRadio.ondisabled = function() {
    FMRadio.ondisabled = null;
    ok(seekCompletes || failedToSeek);
    ok(!FMRadio.enabled);
    finish();
  };
}

verifyInitialState();

