




function sendMessageToJava(message) {
  SpecialPowers.Services.androidBridge.handleGeckoMessage(message);
}

function _evalURI(uri, sandbox) {
  
  
  let req = SpecialPowers.Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                         .createInstance();

  let baseURI = SpecialPowers.Services.io
                             .newURI(window.document.baseURI, window.document.characterSet, null);
  let theURI = SpecialPowers.Services.io
                            .newURI(uri, window.document.characterSet, baseURI);

  
  
  req.open('GET', theURI.spec + ((/\?/).test(theURI.spec) ? "&slug=" : "?slug=") + (new Date()).getTime(), false);
  req.setRequestHeader('Cache-Control', 'no-cache');
  req.setRequestHeader('Pragma', 'no-cache');
  req.send();

  return SpecialPowers.Cu.evalInSandbox(req.responseText, sandbox, "1.8", uri, 1);
}











function testOneFile(uri) {
  let HEAD_JS = "robocop_head.js";

  
  
  
  
  
  
  let principal = SpecialPowers.Cc["@mozilla.org/systemprincipal;1"]
                               .createInstance(SpecialPowers.Ci.nsIPrincipal);

  let testScope = SpecialPowers.Cu.Sandbox(principal);

  
  testScope.Components = SpecialPowers.Components;
  testScope._TEST_FILE = uri;

  
  
  testScope.dump = function (str) {
    let message = { type: "Robocop:JS",
                    innerType: "progress",
                    message: str,
                  };
    sendMessageToJava(message);
  };

  
  
  _evalURI(HEAD_JS, testScope);

  return _evalURI(uri, testScope);
}
