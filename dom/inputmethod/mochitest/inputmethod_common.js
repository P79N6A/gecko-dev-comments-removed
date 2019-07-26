function inputmethod_setup(callback) {
  SimpleTest.waitForExplicitFinish();
  SpecialPowers.Cu.import("resource://gre/modules/Keyboard.jsm", this);

  let permissions = [];
  ['input-manage', 'browser'].forEach(function(name) {
    permissions.push({
      type: name,
      allow: true,
      context: document
    });
  });

  SpecialPowers.pushPermissions(permissions, function() {
    let prefs = [
      ['dom.mozBrowserFramesEnabled', true],
      
      ['dom.mozInputMethod.enabled', true],
      
      ['dom.mozInputMethod.testing', true]
    ];
    SpecialPowers.pushPrefEnv({set: prefs}, callback);
  });
}

function inputmethod_cleanup() {
  SimpleTest.finish();
}
