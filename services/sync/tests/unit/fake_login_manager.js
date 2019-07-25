Cu.import("resource://weave/util.js");





let fakeSampleLogins = [
  
  {hostname: "www.boogle.com",
   formSubmitURL: "http://www.boogle.com/search",
   httpRealm: "",
   username: "",
   password: "",
   usernameField: "test_person",
   passwordField: "test_password"}
];





function FakeLoginManager(fakeLogins) {
  this.fakeLogins = fakeLogins;

  let self = this;

  Utils.getLoginManager = function fake_getLoginManager() {
    
    return {
      removeAllLogins: function() { self.fakeLogins = []; },
      getAllLogins: function() { return self.fakeLogins; },
      addLogin: function(login) {
        getTestLogger().info("nsILoginManager.addLogin() called " +
                             "with hostname '" + login.hostname + "'.");
        self.fakeLogins.push(login);
      }
    };
  };
}
