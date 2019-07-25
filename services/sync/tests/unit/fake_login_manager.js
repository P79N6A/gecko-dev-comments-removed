Cu.import("resource://services-sync/util.js");





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

  
  delete Services.logins;
  Services.logins = {
      removeAllLogins: function() { self.fakeLogins = []; },
      getAllLogins: function() { return self.fakeLogins; },
      addLogin: function(login) {
        getTestLogger().info("nsILoginManager.addLogin() called " +
                             "with hostname '" + login.hostname + "'.");
        self.fakeLogins.push(login);
      }
  };
}
