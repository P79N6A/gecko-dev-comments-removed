


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "icc_header.js";

function testReadContacts(type) {
  let request = icc.readContacts(type);
  request.onsuccess = function onsuccess() {
    let contacts = request.result;

    is(Array.isArray(contacts), true);

    is(contacts[0].name[0], "Mozilla");
    is(contacts[0].tel[0].value, "15555218201");
    is(contacts[0].id, "890141032111185107201");

    is(contacts[1].name[0], "Saßê黃");
    is(contacts[1].tel[0].value, "15555218202");
    is(contacts[1].id, "890141032111185107202");

    is(contacts[2].name[0], "Fire 火");
    is(contacts[2].tel[0].value, "15555218203");
    is(contacts[2].id, "890141032111185107203");

    is(contacts[3].name[0], "Huang 黃");
    is(contacts[3].tel[0].value, "15555218204");
    is(contacts[3].id, "890141032111185107204");

    taskHelper.runNext();
  };

  request.onerror = function onerror() {
    ok(false, "Cannot get " + type + " contacts");
    taskHelper.runNext();
  };
}

function testAddContact(type, pin2) {
  let contact = new mozContact({
    name: ["add"],
    tel: [{value: "0912345678"}],
    email:[]
  });

  let updateRequest = icc.updateContact(type, contact, pin2);

  updateRequest.onsuccess = function onsuccess() {
    

    let getRequest = icc.readContacts(type);

    getRequest.onsuccess = function onsuccess() {
      let contacts = getRequest.result;

      
      is(contacts.length, 5);

      is(contacts[4].name[0], "add");
      is(contacts[4].tel[0].value, "0912345678");

      taskHelper.runNext();
    };

    getRequest.onerror = function onerror() {
      ok(false, "Cannot get " + type + " contacts: " + getRequest.error.name);
      taskHelper.runNext();
    };
  };

  updateRequest.onerror = function onerror() {
    if (type === "fdn" && pin2 === undefined) {
      ok(updateRequest.error.name === "pin2 is empty",
         "expected error when pin2 is not provided");
    } else {
      ok(false, "Cannot add " + type + " contact: " + updateRequest.error.name);
    }
    taskHelper.runNext();
  };
}


taskHelper.push(function testReadAdnContacts() {
  testReadContacts("adn");
});


taskHelper.push(function testAddAdnContact() {
  testAddContact("adn");
});


taskHelper.push(function testReadAdnContacts() {
  testReadContacts("fdn");
});


taskHelper.push(function testReadAdnContacts() {
  testAddContact("fdn", "0000");
});


taskHelper.push(function testReadAdnContacts() {
  testAddContact("fdn");
});


taskHelper.runNext();
