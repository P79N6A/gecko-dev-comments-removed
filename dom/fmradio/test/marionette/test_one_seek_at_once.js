


MARIONETTE_TIMEOUT = 10000;

SpecialPowers.addPermission("fmradio", true, document);

let FMRadio = window.navigator.mozFMRadio;

function verifyInitialState() {
  log("Verifying initial state.");
  ok(FMRadio);
  is(FMRadio.enabled, false);
  setUp();
}

function setUp() {
  let frequency = FMRadio.frequencyLowerBound + FMRadio.channelWidth;
  FMRadio.enable(frequency);
  FMRadio.onenabled = seek;
}

function seek() {
  var request = FMRadio.seekUp();
  ok(request);

  
  
  
  

  request.onerror = function() {
    ok(!firstSeekCompletes);
    cleanUp();
  };

  var firstSeekCompletes = false;
  request.onsuccess = function() {
    firstSeekCompletes = true;
  };

  var seekAgainReq = FMRadio.seekUp();
  ok(seekAgainReq);

  seekAgainReq.onerror = function() {
    log("Cancel the first seek to finish the test");
    let cancelReq = FMRadio.cancelSeek();
    ok(cancelReq);

    
    
    cancelReq.onerror = function() {
      cleanUp();
    };
  };

  seekAgainReq.onsuccess = function() {
    ok(firstSeekCompletes);
    cleanUp();
  };
}

function cleanUp() {
  FMRadio.disable();
  FMRadio.ondisabled = function() {
    FMRadio.ondisabled = null;
    ok(!FMRadio.enabled);
    finish();
  };
}

verifyInitialState();

