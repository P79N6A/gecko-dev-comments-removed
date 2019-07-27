


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function testReadContacts(aIcc, aType) {
  log("testReadContacts: type=" + aType);
  let iccId = aIcc.iccInfo.iccid;
  return aIcc.readContacts(aType)
    .then((aResult) => {
      is(Array.isArray(aResult), true);
      is(aResult.length, 4, "Check contact number.");

      
      is(aResult[0].name[0], "Mozilla");
      is(aResult[0].tel[0].value, "15555218201");
      is(aResult[0].id, iccId + "1");

      
      is(aResult[1].name[0], "Saßê黃");
      is(aResult[1].tel[0].value, "15555218202");
      is(aResult[1].id, iccId + "2");

      
      is(aResult[2].name[0], "Fire 火");
      is(aResult[2].tel[0].value, "15555218203");
      is(aResult[2].id, iccId + "3");

      
      is(aResult[3].name[0], "Huang 黃");
      is(aResult[3].tel[0].value, "15555218204");
      is(aResult[3].id, iccId + "4");
    }, (aError) => {
      ok(false, "Cannot get " + aType + " contacts");
    });
}



startTestCommon(function() {
  let icc = getMozIcc();

  
  return testReadContacts(icc, "adn")
    
    .then(() => testReadContacts(icc, "fdn"))
    
    .then(() => testReadContacts(icc, "sdn"));
});
