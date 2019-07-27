


MARIONETTE_TIMEOUT = 90000;
MARIONETTE_HEAD_JS = "head.js";

let TEST_ADD_DATA = [{
    
    name: ["add1"],
    tel: [{value: "0912345678"}],
  }, {
    
    name: ["add2"],
    tel: [{value: "012345678901234567890123456789"}],
  }, {
    
    name: ["add3"],
    tel: [{value: "01234567890123456789"}],
    email:[{value: "test@mozilla.com"}],
  }, {
    
    name: ["add4"],
    tel: [{value: "01234567890123456789"}, {value: "123456"}, {value: "123"}],
  }, {
    
    name: ["add5"],
    tel: [{value: "01234567890123456789"}, {value: "123456"}, {value: "123"}],
    email:[{value: "test@mozilla.com"}],
  }];

function testAddContact(aIcc, aType, aMozContact, aPin2) {
  log("testAddContact: type=" + aType + ", pin2=" + aPin2);
  let contact = new mozContact(aMozContact);

  return aIcc.updateContact(aType, contact, aPin2)
    .then((aResult) => {
      is(aResult.name[0], aMozContact.name[0]);
      
      is(aResult.tel[0].value, aMozContact.tel[0].value.substring(0, 20));
      
      ok(aResult.tel.length == 1);
      ok(!aResult.email);

      
      return aIcc.readContacts(aType)
        .then((aResult) => {
          let contact = aResult[aResult.length - 1];

          is(contact.name[0], aMozContact.name[0]);
          
          is(contact.tel[0].value, aMozContact.tel[0].value.substring(0, 20));
          is(contact.id, aIcc.iccInfo.iccid + aResult.length);

          return contact.id;
        })
        .then((aContactId) => {
          
          return removeContact(aIcc, aContactId, aType, aPin2);
        });
    }, (aError) => {
      if (aType === "fdn" && aPin2 === undefined) {
        ok(aError.name === "SimPin2",
           "expected error when pin2 is not provided");
      } else {
        ok(false, "Cannot add " + aType + " contact: " + aError.name);
      }
    })
}

function removeContact(aIcc, aContactId, aType, aPin2) {
  log("removeContact: contactId=" + aContactId +
      ", type=" + aType + ", pin2=" + aPin2);

  let contact = new mozContact({});
  contact.id = aIcc.iccInfo.iccid + aContactId;

  return aIcc.updateContact(aType, contact, aPin2);
}


startTestCommon(function() {
  let icc = getMozIcc();

  for (let i = 0; i < TEST_ADD_DATA.length; i++) {
    let test_data = TEST_ADD_DATA[i];
    
    return testAddContact(icc, "adn", test_data)
      
      .then(() => testAddContact(icc, "fdn", test_data, "0000"))
      
      .then(() => testAddContact(icc, "fdn", test_data));
  }
});
