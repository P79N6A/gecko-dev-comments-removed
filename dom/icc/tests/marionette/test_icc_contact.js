


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function testReadContacts(aIcc, aType) {
  log("testReadContacts: type=" + aType);
  let iccId = aIcc.iccInfo.iccid;
  return aIcc.readContacts(aType)
    .then((aResult) => {
      is(Array.isArray(aResult), true);

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

function testAddContact(aIcc, aType, aPin2) {
  log("testAddContact: type=" + aType + ", pin2=" + aPin2);
  let contact = new mozContact({
    name: ["add"],
    tel: [{value: "0912345678"}],
    email:[]
  });

  return aIcc.updateContact(aType, contact, aPin2)
    .then((aResult) => {
      is(aResult.id, aIcc.iccInfo.iccid + "5");
      is(aResult.name[0], "add");
      is(aResult.tel[0].value, "0912345678");
      
      return aIcc.readContacts(aType)
        .then((aResult) => {
          
          is(aResult.length, 5);

          is(aResult[4].name[0], "add");
          is(aResult[4].tel[0].value, "0912345678");
          is(aResult[4].id, aIcc.iccInfo.iccid + "5");
        }, (aError) => {
          ok(false, "Cannot get " + aType + " contacts: " + aError.name);
        })
    }, (aError) => {
      if (aType === "fdn" && aPin2 === undefined) {
        ok(aError.name === "SimPin2",
           "expected error when pin2 is not provided");
      } else {
        ok(false, "Cannot add " + aType + " contact: " + aError.name);
      }
    });
}


startTestCommon(function() {
  let icc = getMozIcc();

  
  return testReadContacts(icc, "adn")
    
    .then(() => testAddContact(icc, "adn"))
    
    .then(() => testReadContacts(icc, "fdn"))
    
    .then(() => testAddContact(icc, "fdn", "0000"))
    
    .then(() => testAddContact(icc, "fdn"))
    
    .then(() => testReadContacts(icc, "sdn"));
});
