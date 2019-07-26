



function test() {
  waitForExplicitFinish();

  let manifest = { 
    name: "provider 1",
    origin: "https://example.com",
    workerURL: "https://example.com/browser/browser/base/content/test/social_worker.js",
    iconURL: "https://example.com/browser/browser/base/content/test/moz.png"
  };
  runSocialTestWithProvider(manifest, function (finishcb) {
    runSocialTests(tests, undefined, undefined, finishcb);
  });
}

var tests = {
  testProfileSet: function(next) {
    let profile = {
      portrait: "https://example.com/portrait.jpg",
      userName: "trickster",
      displayName: "Kuma Lisa",
      profileURL: "http://en.wikipedia.org/wiki/Kuma_Lisa"
    }
    Social.provider.updateUserProfile(profile);
    
    let portrait = document.getElementById("social-statusarea-user-portrait").getAttribute("src");
    is(profile.portrait, portrait, "portrait is set");
    let userButton = document.getElementById("social-statusarea-username");
    ok(!userButton.hidden, "username is visible");
    is(userButton.value, profile.userName, "username is set");
    next();
  },
  testAmbientNotifications: function(next) {
    let ambience = {
      name: "testIcon",
      iconURL: "https://example.com/browser/browser/base/content/test/moz.png",
      contentPanel: "about:blank",
      counter: 42
    };
    Social.provider.setAmbientNotification(ambience);

    let statusIcon = document.querySelector("#social-toolbar-item > box");
    waitForCondition(function() {
      statusIcon = document.querySelector("#social-toolbar-item > box");
      return !!statusIcon;
    }, function () {
      let statusIconLabel = statusIcon.querySelector("label");
      is(statusIconLabel.value, "42", "status value is correct");

      ambience.counter = 0;
      Social.provider.setAmbientNotification(ambience);
      is(statusIconLabel.value, "", "status value is correct");
      next();
    }, "statusIcon was never found");
  },
  testProfileUnset: function(next) {
    Social.provider.updateUserProfile({});
    
    let userButton = document.getElementById("social-statusarea-username");
    ok(userButton.hidden, "username is not visible");
    let ambientIcons = document.querySelectorAll("#social-toolbar-item > box");
    for (let ambientIcon of ambientIcons) {
      ok(ambientIcon.collapsed, "ambient icon (" + ambientIcon.id + ") is collapsed");
    }
    
    next();
  },
  testShowSidebarMenuitemExists: function(next) {
    let toggleSidebarMenuitem = document.getElementById("social-toggle-sidebar-menuitem");
    ok(toggleSidebarMenuitem, "Toggle Sidebar menuitem exists");
    next();
  },
  testShowDesktopNotificationsMenuitemExists: function(next) {
    let toggleDesktopNotificationsMenuitem = document.getElementById("social-toggle-notifications-menuitem");
    ok(toggleDesktopNotificationsMenuitem, "Toggle notifications menuitem exists");
    next();
  }
}

