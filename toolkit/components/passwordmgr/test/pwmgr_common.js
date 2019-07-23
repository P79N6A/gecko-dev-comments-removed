




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
    
    netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');

    var keyName = "DOM_VK_" + aKey.toUpperCase();
    var key = Components.interfaces.nsIDOMKeyEvent[keyName];

    
    if (!modifier)
        modifier = null;

    
    var wutils = window.QueryInterface(Components.interfaces.nsIInterfaceRequestor).
                          getInterface(Components.interfaces.nsIDOMWindowUtils);

    wutils.sendKeyEvent("keydown",  key, 0, modifier);
    wutils.sendKeyEvent("keypress", key, 0, modifier);
    wutils.sendKeyEvent("keyup",    key, 0, modifier);
}


function commonInit() {
    netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');

    var pwmgr = Components.classes["@mozilla.org/login-manager;1"].
                getService(Components.interfaces.nsILoginManager);
    ok(pwmgr != null, "Access LoginManager");


    
    var logins = pwmgr.getAllLogins({});
    if (logins.length) {
        
        pwmgr.removeAllLogins();
    }
    var disabledHosts = pwmgr.getAllDisabledHosts({});
    if (disabledHosts.length) {
        
        for each (var host in disabledHosts)
            pwmgr.setLoginSavingEnabled(host, true);
    }

    
    var login = Components.classes["@mozilla.org/login-manager/loginInfo;1"].
                createInstance(Components.interfaces.nsILoginInfo);
    login.init("http://localhost:8888", "http://localhost:8888", null,
               "testuser", "testpass", "uname", "pword");
    pwmgr.addLogin(login);

    
    logins = pwmgr.getAllLogins({});
    is(logins.length, 1, "Checking for successful init login");
    disabledHosts = pwmgr.getAllDisabledHosts({});
    is(disabledHosts.length, 0, "Checking for no disabled hosts");
}
