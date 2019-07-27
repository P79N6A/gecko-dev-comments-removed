




function $_(formNum, name) {
  var form = document.getElementById("form" + formNum);
  if (!form) {
    logWarning("$_ couldn't find requested form " + formNum);
    return null;
  }

  var element = form.elements.namedItem(name);
  if (!element) {
    logWarning("$_ couldn't find requested element " + name);
    return null;
  }

  
  
  
  
  

  if (element.getAttribute("name") != name) {
    logWarning("$_ got confused.");
    return null;
  }

  return element;
}











function checkForm(formNum, val1, val2, val3) {
    var e, form = document.getElementById("form" + formNum);
    ok(form, "Locating form " + formNum);

    var numToCheck = arguments.length - 1;
    
    if (!numToCheck--)
        return;
    e = form.elements[0];
    if (val1 == null)
        is(e.value, e.defaultValue, "Test default value of field " + e.name +
            " in form " + formNum);
    else
        is(e.value, val1, "Test value of field " + e.name +
            " in form " + formNum);


    if (!numToCheck--)
        return;
    e = form.elements[1];
    if (val2 == null)
        is(e.value, e.defaultValue, "Test default value of field " + e.name +
            " in form " + formNum);
    else
        is(e.value, val2, "Test value of field " + e.name +
            " in form " + formNum);


    if (!numToCheck--)
        return;
    e = form.elements[2];
    if (val3 == null)
        is(e.value, e.defaultValue, "Test default value of field " + e.name +
            " in form " + formNum);
    else
        is(e.value, val3, "Test value of field " + e.name +
            " in form " + formNum);

}










function checkUnmodifiedForm(formNum) {
    var form = document.getElementById("form" + formNum);
    ok(form, "Locating form " + formNum);

    for (var i = 0; i < form.elements.length; i++) {
        var ele = form.elements[i];

        
        if (ele.type == "submit" || ele.type == "reset")
            continue;

        is(ele.value, ele.defaultValue, "Test to default value of field " +
            ele.name + " in form " + formNum);
    }
}




function doKey(aKey, modifier) {
    var keyName = "DOM_VK_" + aKey.toUpperCase();
    var key = KeyEvent[keyName];

    
    if (!modifier)
        modifier = null;

    
    var wutils = SpecialPowers.wrap(window).
                          QueryInterface(SpecialPowers.Ci.nsIInterfaceRequestor).
                          getInterface(SpecialPowers.Ci.nsIDOMWindowUtils);

    if (wutils.sendKeyEvent("keydown",  key, 0, modifier)) {
      wutils.sendKeyEvent("keypress", key, 0, modifier);
    }
    wutils.sendKeyEvent("keyup",    key, 0, modifier);
}





function commonInit(selfFilling) {
    var pwmgr = SpecialPowers.Cc["@mozilla.org/login-manager;1"].
                getService(SpecialPowers.Ci.nsILoginManager);
    ok(pwmgr != null, "Access LoginManager");


    
    var logins = pwmgr.getAllLogins();
    if (logins.length) {
        
        pwmgr.removeAllLogins();
    }
    var disabledHosts = pwmgr.getAllDisabledHosts();
    if (disabledHosts.length) {
        
        for (var host of disabledHosts)
            pwmgr.setLoginSavingEnabled(host, true);
    }

    
    var login = SpecialPowers.Cc["@mozilla.org/login-manager/loginInfo;1"].
                createInstance(SpecialPowers.Ci.nsILoginInfo);
    login.init("http://mochi.test:8888", "http://mochi.test:8888", null,
               "testuser", "testpass", "uname", "pword");
    pwmgr.addLogin(login);

    
    logins = pwmgr.getAllLogins();
    is(logins.length, 1, "Checking for successful init login");
    disabledHosts = pwmgr.getAllDisabledHosts();
    is(disabledHosts.length, 0, "Checking for no disabled hosts");

    if (selfFilling)
        return;

    
    
    
    
    window.addEventListener("DOMContentLoaded", (event) => {
        var form = document.createElement('form');
        form.id = 'observerforcer';
        var username = document.createElement('input');
        username.name = 'testuser';
        form.appendChild(username);
        var password = document.createElement('input');
        password.name = 'testpass';
        password.type = 'password';
        form.appendChild(password);

        var observer = SpecialPowers.wrapCallback(function(subject, topic, data) {
            var formLikeRoot = subject.QueryInterface(SpecialPowers.Ci.nsIDOMNode);
            if (formLikeRoot.id !== 'observerforcer')
                return;
            SpecialPowers.removeObserver(observer, "passwordmgr-processed-form");
            formLikeRoot.remove();
            SimpleTest.executeSoon(() => {
                var event = new Event("runTests");
                window.dispatchEvent(event);
            });
        });
        SpecialPowers.addObserver(observer, "passwordmgr-processed-form", false);

        document.body.appendChild(form);
    });
}

const masterPassword = "omgsecret!";

function enableMasterPassword() {
    setMasterPassword(true);
}

function disableMasterPassword() {
    setMasterPassword(false);
}

function setMasterPassword(enable) {
    var oldPW, newPW;
    if (enable) {
        oldPW = "";
        newPW = masterPassword;
    } else {
        oldPW = masterPassword;
        newPW = "";
    }
    
    

    var pk11db = Cc["@mozilla.org/security/pk11tokendb;1"].
                 getService(Ci.nsIPK11TokenDB)
    var token = pk11db.findTokenByName("");
    ok(true, "change from " + oldPW + " to " + newPW);
    token.changePassword(oldPW, newPW);
}

function logoutMasterPassword() {
    var sdr = Cc["@mozilla.org/security/sdr;1"].
            getService(Ci.nsISecretDecoderRing);
    sdr.logoutAndTeardown();
}

function dumpLogins(pwmgr) {
    var logins = pwmgr.getAllLogins();
    ok(true, "----- dumpLogins: have " + logins.length + " logins. -----");
    for (var i = 0; i < logins.length; i++)
        dumpLogin("login #" + i + " --- ", logins[i]);
}

function dumpLogin(label, login) {
    loginText = "";
    loginText += "host: ";
    loginText += login.hostname;
    loginText += " / formURL: ";
    loginText += login.formSubmitURL;
    loginText += " / realm: ";
    loginText += login.httpRealm;
    loginText += " / user: ";
    loginText += login.username;
    loginText += " / pass: ";
    loginText += login.password;
    loginText += " / ufield: ";
    loginText += login.usernameField;
    loginText += " / pfield: ";
    loginText += login.passwordField;
    ok(true, label + loginText);
}




function promiseFormsProcessed(expectedCount = 1) {
  var processedCount = 0;
  return new Promise((resolve, reject) => {
    function onProcessedForm(subject, topic, data) {
      processedCount++;
      if (processedCount == expectedCount) {
        SpecialPowers.removeObserver(onProcessedForm, "passwordmgr-processed-form");
        resolve(subject, data);
      }
    }
    SpecialPowers.addObserver(onProcessedForm, "passwordmgr-processed-form", false);
  });
}


if (this.addMessageListener) {
  const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;
  var SpecialPowers = { Cc, Ci, Cr, Cu, };
  var ok, is;
  
  ok = is = () => {};

  Cu.import("resource://gre/modules/Task.jsm");

  addMessageListener("setupParent", () => {
    commonInit(true);
    sendAsyncMessage("doneSetup");
  });

  addMessageListener("loadRecipes", Task.async(function* loadRecipes(recipes) {
    var { LoginManagerParent } = Cu.import("resource://gre/modules/LoginManagerParent.jsm", {});
    var recipeParent = yield LoginManagerParent.recipeParentPromise;
    yield recipeParent.load(recipes);
    sendAsyncMessage("loadedRecipes", recipes);
  }));

  var globalMM = Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIMessageListenerManager);
  globalMM.addMessageListener("RemoteLogins:onFormSubmit", function onFormSubmit(message) {
    sendAsyncMessage("formSubmissionProcessed", message.data, message.objects);
  });
} else {
  
  SimpleTest.registerCleanupFunction(() => {
    var { LoginManagerParent } = SpecialPowers.Cu.import("resource://gre/modules/LoginManagerParent.jsm", {});
    return LoginManagerParent.recipeParentPromise.then((recipeParent) => {
      SpecialPowers.wrap(recipeParent).reset();
    });
  });
}
