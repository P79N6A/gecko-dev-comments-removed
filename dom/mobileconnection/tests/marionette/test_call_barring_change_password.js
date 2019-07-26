


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  

  
  [null, "0000", "InvalidPassword"],
  ["0000", null, "InvalidPassword"],
  [null, null, "InvalidPassword"],

  
  ["000", "0000", "InvalidPassword"],
  ["00000", "1111", "InvalidPassword"],
  ["abcd", "efgh", "InvalidPassword"],

  
  
  
  ["1234", "1234", "RequestNotSupported"]
];

function testChangeCallBarringPassword(aPin, aNewPin, aExpectedError) {
  log("Test changing call barring password to " + aPin + "/" + aNewPin);

  let options = {
    pin: aPin,
    newPin: aNewPin
  };
  return changeCallBarringPassword(options)
    .then(function resolve() {
      ok(!aExpectedError, "changeCallBarringPassword success");
    }, function reject(aError) {
      is(aError.name, aExpectedError, "failed to changeCallBarringPassword");
    });
}


startTestCommon(function() {
  let promise = Promise.resolve();
  for (let i = 0; i < TEST_DATA.length; i++) {
    let data = TEST_DATA[i];
    promise =
      promise.then(() => testChangeCallBarringPassword(data[0], data[1], data[2]));
  }
  return promise;
});
