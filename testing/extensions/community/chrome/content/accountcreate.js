





































var accountCreate = {
    oldPassword: null,    
    oldUsername: null,
    updateFunction : null,
  loadSetup : function() {
    document.getElementById('qa-settings-createaccount-frame').src =
    litmus.baseURL+'extension.cgi?createAccount=1';
        accountCreate.updateFunction = window.arguments[0];

        accountCreate.oldPassword = qaPref.litmus.getPassword();
        accountCreate.oldUsername = qaPref.litmus.getUsername();
  },
    validateAccount : function() {

    var account = qaSetup.retrieveAccount("qa-settings-createaccount-frame");
    var uname = account.name;
    var passwd = account.password;


        if (uname == '' || passwd == '') {
            alert("No username or password has been registered.");
            return false;    
        }

    var location = document.getElementById('qa-settings-createaccount-frame').contentDocument.location + "";
    if (location.indexOf("createAccount") != -1) {
      alert("Account not created! You most likely need to click the 'Create Account' button");
      return false;
    }

        qaPref.litmus.setPassword(uname, passwd);
        accountCreate.updateFunction();

      
    return true;
  },

    handleCancel : function() {
        qaPref.litmus.setPassword(oldUsername, oldPassword);
    }
}
