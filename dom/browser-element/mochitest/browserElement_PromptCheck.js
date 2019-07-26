








"use strict";

SimpleTest.waitForExplicitFinish();

browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();



const dialogTimeLimitPrefName = 'dom.successive_dialog_time_limit';
var oldDialogTimeLimitPref;
try {
  oldDialogTimeLimitPref = SpecialPowers.getIntPref(dialogTimeLimitPrefName);
}
catch(e) {}

SpecialPowers.setIntPref(dialogTimeLimitPrefName, 10);

var iframe = document.createElement('iframe');
SpecialPowers.wrap(iframe).mozbrowser = true;
document.body.appendChild(iframe);

var numPrompts = 0;
iframe.addEventListener('mozbrowsershowmodalprompt', function(e) {
  is(e.detail.message, numPrompts, "prompt message");
  if (numPrompts / 10 < 1) {
    is(e.detail.promptType, 'alert');
  }
  else if (numPrompts / 10 < 2) {
    is(e.detail.promptType, 'confirm');
  }
  else {
    is(e.detail.promptType, 'prompt');
  }

  numPrompts++;
  if (numPrompts == 30) {
    if (oldDialogTimeLimitPref !== undefined) {
      SpecialPowers.setIntPref(dialogTimeLimitPrefName, oldDialogTimeLimitPref);
    }
    else {
      SpecialPowers.clearUserPref(dialogTimeLimitPrefName);
    }

    SimpleTest.finish();
  }
});

iframe.src =
  'data:text/html,<html><body><script>\
    var i = 0; \
    for (; i < 10; i++) { alert(i); } \
    for (; i < 20; i++) { confirm(i); } \
    for (; i < 30; i++) { prompt(i); } \
   </scr' + 'ipt></body></html>';
