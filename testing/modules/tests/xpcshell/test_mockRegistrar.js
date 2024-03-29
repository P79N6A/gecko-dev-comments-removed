


const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://testing-common/MockRegistrar.jsm");

function userInfo(username) {
  this.username = username;
}

userInfo.prototype = {
  fullname: "fullname",
  emailAddress: "emailAddress",
  domain: "domain",
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIUserInfo]),
};

function run_test () {
  run_next_test();
}

add_test(function test_register() {
  let localUserInfo = {
    fullname: "fullname",
    username: "localusername",
    emailAddress: "emailAddress",
    domain: "domain",
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIUserInfo]),
  };

  let userInfoCID = MockRegistrar.register("@mozilla.org/userinfo;1", localUserInfo);
  Assert.equal(Cc["@mozilla.org/userinfo;1"].createInstance(Ci.nsIUserInfo).username, "localusername");
  run_next_test();
});

add_test(function test_register_with_arguments() {
  let userInfoCID = MockRegistrar.register("@mozilla.org/userinfo;1", userInfo, ["username"]);
  Assert.equal(Cc["@mozilla.org/userinfo;1"].createInstance(Ci.nsIUserInfo).username, "username");
  run_next_test();
});

add_test(function test_register_twice() {
  let userInfoCID = MockRegistrar.register("@mozilla.org/userinfo;1", userInfo, ["originalname"]);
  Assert.equal(Cc["@mozilla.org/userinfo;1"].createInstance(Ci.nsIUserInfo).username, "originalname");

  let newUserInfoCID = MockRegistrar.register("@mozilla.org/userinfo;1", userInfo, ["newname"]);
  Assert.equal(Cc["@mozilla.org/userinfo;1"].createInstance(Ci.nsIUserInfo).username, "newname");
  run_next_test();
});
