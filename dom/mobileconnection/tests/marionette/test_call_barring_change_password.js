


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function testChangeCallBarringPassword(aExpectedError, aOptions) {
  log("Test changing call barring password from " +
      aOptions.pin + " to " + aOptions.newPin);

  return changeCallBarringPassword(aOptions)
    .then(function resolve() {
      ok(!aExpectedError, "changeCallBarringPassword success");
    }, function reject(aError) {
      is(aError.name, aExpectedError, "failed to changeCallBarringPassword");
    });
}


startTestCommon(function() {
  return Promise.resolve()

    
    

    
    .then(() => testChangeCallBarringPassword("InvalidPassword", {
      pin: null,
      newPin: "0000"
    }))

    .then(() => testChangeCallBarringPassword("InvalidPassword", {
      pin: "000",
      newPin: "0000"
    }))

    .then(() => testChangeCallBarringPassword("InvalidPassword", {
      pin: "00000",
      newPin: "0000"
    }))

    .then(() => testChangeCallBarringPassword("InvalidPassword", {
      pin: "abcd",
      newPin: "0000"
    }))

    
    .then(() => testChangeCallBarringPassword("InvalidPassword", {
      pin: "0000",
      newPin: null
    }))

    .then(() => testChangeCallBarringPassword("InvalidPassword", {
      pin: "0000",
      newPin: "000"
    }))

    .then(() => testChangeCallBarringPassword("InvalidPassword", {
      pin: "0000",
      newPin: "00000"
    }))

    .then(() => testChangeCallBarringPassword("InvalidPassword", {
      pin: "0000",
      newPin: "abcd"
    }))

    
    .then(() => testChangeCallBarringPassword("IncorrectPassword", {
      pin: "1111",
      newPin: "2222"
    }))

    
    .then(() => testChangeCallBarringPassword(null, {
      pin: "0000",
      newPin: "2222"
    }))

    .then(() => testChangeCallBarringPassword(null, {
      pin: "2222",
      newPin: "0000"
    }));
});
