


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "mobile_header.js";

function doSetAndVerifyPreferredNetworkType(preferredNetworkType, callback) {
  log("setPreferredNetworkType to '" + preferredNetworkType + "'.");
  let setRequest = mobileConnection.setPreferredNetworkType(preferredNetworkType);
  ok(setRequest instanceof DOMRequest,
     "setRequest instanceof " + setRequest.constructor);

  setRequest.onsuccess = function() {
    log("Verify preferred network.");
    let getRequest = mobileConnection.getPreferredNetworkType();
    ok(getRequest instanceof DOMRequest,
       "getRequest instanceof " + getRequest.constructor);

    getRequest.onsuccess = function() {
      is(getRequest.result, preferredNetworkType, "Check preferred network type.");
      callback();
    };

    getRequest.onerror = function() {
      ok(false, "getPreferredNetworkType got error: " + getRequest.error.name);
      callback();
    };
  };

  setRequest.onerror = function() {
    ok(false, "setPreferredNetwork got error: " + setRequest.error.name);
    callback();
  };
}

function doFailToSetPreferredNetworkType(preferredNetworkType, expectedError, callback) {
  log("setPreferredNetworkType to '" + preferredNetworkType + "'.");
  let request = mobileConnection.setPreferredNetworkType(preferredNetworkType);
  ok(request instanceof DOMRequest,
     "request instanceof " + request.constructor);

  request.onsuccess = function() {
    ok(false, "Should not success");
    callback();
  };

  request.onerror = function() {
    is(request.error.name, expectedError, "Check error message.");
    callback();
  };
}


taskHelper.push(function testSupportedNetworkTypes() {
  let supportedNetworkTypes = mobileConnection.supportedNetworkTypes;

  ok(Array.isArray(supportedNetworkTypes), "supportedNetworkTypes should be an array");
  ok(supportedNetworkTypes.indexOf("gsm") >= 0, "Should support 'gsm'");
  ok(supportedNetworkTypes.indexOf("wcdma") >= 0, "Should support 'wcdma'");
  ok(supportedNetworkTypes.indexOf("cdma") >= 0, "Should support 'cdma'");
  ok(supportedNetworkTypes.indexOf("evdo") >= 0, "Should support 'evdo'");

  taskHelper.runNext();
});


taskHelper.push(function testPreferredNetworkTypes() {
  let supportedTypes = [
    'gsm',
    'wcdma',
    'wcdma/gsm-auto',
    'cdma/evdo',
    'evdo',
    'cdma',
    'wcdma/gsm/cdma/evdo',
    
    'wcdma/gsm'
  ];

  
  (function do_call() {
    let type = supportedTypes.shift();
    if (!type) {
      taskHelper.runNext();
      return;
    }
    doSetAndVerifyPreferredNetworkType(type, do_call);
  })();
});


taskHelper.push(function testUnsupportedPreferredNetworkTypes() {
  
  let unsupportedTypes = [
    'lte/cdma/evdo',
    'lte/wcdma/gsm',
    'lte/wcdma/gsm/cdma/evdo',
    'lte'
  ];

  
  (function do_call() {
    let type = unsupportedTypes.shift();
    if (!type) {
      taskHelper.runNext();
      return;
    }
    doFailToSetPreferredNetworkType(type, "ModeNotSupported", do_call);
  })();
});


taskHelper.push(function testInvalidPreferredNetworkTypes() {
  let invalidTypes = [
    ' ',
    'AnInvalidType'
  ];

  
  (function do_call() {
    let type = invalidTypes.shift();
    if (!type) {
      taskHelper.runNext();
      return;
    }
    doFailToSetPreferredNetworkType(type, "InvalidParameter", do_call);
  })();
});


taskHelper.runNext();
