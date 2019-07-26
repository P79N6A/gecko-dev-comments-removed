


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function check(aLongName, aShortName, aRoaming) {
  let voice = mobileConnection.voice;
  let network = voice.network;

  is(network.longName, aLongName, "network.longName");
  is(network.shortName, aShortName, "network.shortName");
  is(voice.roaming, aRoaming, "voice.roaming");
}









function test(aLongName, aShortName, aRoaming) {
  log("Testing roaming check '" + aLongName + "', '" + aShortName + "':");

  return Promise.resolve()

    
    .then(() => setEmulatorOperatorNamesAndWait("roaming", aLongName, aShortName,
                                                null, null, true, false))

    .then(() => setEmulatorVoiceDataStateAndWait("voice", "roaming"))
    .then(() => check(aLongName, aShortName, aRoaming))
    .then(() => setEmulatorVoiceDataStateAndWait("voice", "home"));
}

startTestCommon(function() {
  return getEmulatorOperatorNames()
    .then(function(aOperators) {
      let {longName: longName, shortName: shortName} = aOperators[0];

      return Promise.resolve()

        
        
        
        .then(() => test(longName,               shortName,               false))
        .then(() => test(longName,               shortName.toLowerCase(), false))
        .then(() => test(longName,               "Bar",                   false))
        .then(() => test(longName.toLowerCase(), shortName,               false))
        .then(() => test(longName.toLowerCase(), shortName.toLowerCase(), false))
        .then(() => test(longName.toLowerCase(), "Bar",                   false))
        .then(() => test("Foo",                  shortName,               false))
        .then(() => test("Foo",                  shortName.toLowerCase(), false))
        .then(() => test("Foo",                  "Bar",                   true))

        
        .then(() => setEmulatorOperatorNamesAndWait("roaming",
                                                    aOperators[1].longName,
                                                    aOperators[1].shortName));
    });
});
