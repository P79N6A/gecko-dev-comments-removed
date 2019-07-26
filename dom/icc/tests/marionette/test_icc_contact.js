


MARIONETTE_TIMEOUT = 30000;

SpecialPowers.addPermission("mobileconnection", true, document);


SpecialPowers.addPermission("contacts-read", true, document);
SpecialPowers.addPermission("contacts-write", true, document);
SpecialPowers.addPermission("contacts-create", true, document);

let icc = navigator.mozMobileConnection.icc;
ok(icc instanceof MozIccManager, "icc is instanceof " + icc.constructor);


let mozContacts = window.navigator.mozContacts;
ok(mozContacts);

function testAddIccContact() {
  let contact = new mozContact();

  contact.init({
    name: "add",
    tel: [{value: "0912345678"}]
  });

  
  let updateRequest = icc.updateContact("ADN", contact);

  updateRequest.onsuccess = function onsuccess() {
    

    
    
    let getRequest = mozContacts.getSimContacts("ADN");

    getRequest.onsuccess = function onsuccess() {
      let simContacts = getRequest.result;

      
      is(simContacts.length, 5);

      is(simContacts[4].name, "add");
      is(simContacts[4].tel[0].value, "0912345678");

      runNextTest();
    };

    getRequest.onerror = function onerror() {
      ok(false, "Cannot get ICC contacts: " + getRequest.error.name);
      runNextTest();
    };
  };

  updateRequest.onerror = function onerror() {
    ok(false, "Cannot add ICC contact: " + updateRequest.error.name);
    runNextTest();
  };
};

let tests = [
  testAddIccContact,
];

function runNextTest() {
  let test = tests.pop();
  if (!test) {
    cleanUp();
    return;
  }

  test();
}

function cleanUp() {
  SpecialPowers.removePermission("mobileconnection", document);

  
  SpecialPowers.removePermission("contacts-read", document);
  SpecialPowers.removePermission("contacts-write", document);
  SpecialPowers.removePermission("contacts-create", document);

  finish();
}

runNextTest();
