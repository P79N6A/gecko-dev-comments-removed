


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_CELL_ID = 268435399; 
                                
                                
                                

function check(aLongName, aShortName, aMcc, aMnc, aLac, aCid) {
  let network = mobileConnection.voice.network;
  log("  Got longName '" + network.longName + "', shortName '" +
      network.shortName + "'");

  is(network.longName, aLongName, "network.longName");
  is(network.shortName, aShortName, "network.shortName");
  is(network.mcc, aMcc, "network.mcc");
  is(network.mnc, aMnc, "network.mnc");

  let cell = mobileConnection.voice.cell;

  is(cell.gsmLocationAreaCode, aLac, "cell.gsmLocationAreaCode");
  is(cell.gsmCellId, aCid, "cell.gsmCellId");
}

function test(aLongName, aShortName, aMcc, aMnc, aLac, aCid,
              aExpectedLongName, aExpectedShortName) {
  log("Testing mcc = " + aMcc + ", mnc = " + aMnc + ", lac = " + aLac + ":");

  return setEmulatorGsmLocationAndWait(aLac, aCid)
    .then(() => setEmulatorOperatorNamesAndWait("home", aLongName, aShortName,
                                                aMcc, aMnc, true, false))
    
    .then(() => check(aExpectedLongName == null ? aLongName : aExpectedLongName,
                      aExpectedShortName == null ? aShortName : aExpectedShortName,
                      aMcc, aMnc, aLac, aCid));
}

startTestCommon(function() {
  
















  return getEmulatorOperatorNames()
    .then(function(aOperators) {
      let {longName: longName, shortName: shortName} = aOperators[0];
      let {mcc: mcc, mnc: mnc} = mobileConnection.voice.network;
      let {gsmLocationAreaCode: lac, gsmCellId: cid} = mobileConnection.voice.cell;

      
      
      isnot(TEST_CELL_ID, cid, "A different test cell id than used currently.");

      
      
      return Promise.resolve()

        
        .then(() => test("Foo1", "Bar1", "123", "456", 0x0000, TEST_CELL_ID))
        .then(() => test("Foo2", "Bar2", "123", "456", 0x0001, TEST_CELL_ID))
        .then(() => test("Foo3", "Bar3", "123", "456", 0x0002, TEST_CELL_ID))
        .then(() => test("Foo4", "Bar4", "123", "456", 0x0010, TEST_CELL_ID))
        .then(() => test("Foo5", "Bar5", "123", "456", 0x0011, TEST_CELL_ID))
        .then(() => test("Foo6", "Bar6", "123", "456", 0xFFFE, TEST_CELL_ID))

        
        .then(() => test("Foo1", "Bar1", "001", "01", 0x0000, TEST_CELL_ID,
                         "Test1", "Test1"))
        .then(() => test("Foo2", "Bar2", "001", "01", 0x0001, TEST_CELL_ID,
                         "Test1", "Test1"))
        .then(() => test("Foo3", "Bar3", "001", "01", 0xFFFE, TEST_CELL_ID,
                         "Test1", "Test1"))

        
        
        .then(() => test("Foo1", "Bar1", "001", "02", 0x0000, TEST_CELL_ID))
        .then(() => test("Foo2", "Bar2", "001", "02", 0x0001, TEST_CELL_ID,
                         "Test2", ""))
        .then(() => test("Foo3", "Bar3", "001", "02", 0x0002, TEST_CELL_ID,
                         "Test2", ""))
        .then(() => test("Foo4", "Bar4", "001", "02", 0x0010, TEST_CELL_ID,
                         "Test2", ""))
        .then(() => test("Foo5", "Bar5", "001", "02", 0xFFFE, TEST_CELL_ID))

        
        .then(() => test("Foo1", "Bar1", "001", "03", 0x0000, TEST_CELL_ID))
        .then(() => test("Foo2", "Bar2", "001", "03", 0x0011, TEST_CELL_ID,
                         "Test3", ""))
        .then(() => test("Foo3", "Bar3", "001", "03", 0xFFFE, TEST_CELL_ID))

        
        .then(() => test("Foo1", "Bar1", "001", "001", 0x0012, TEST_CELL_ID,
                         "Test4", ""))

        
        .then(() => test(longName, shortName, mcc, mnc, lac, cid));
    });
});
