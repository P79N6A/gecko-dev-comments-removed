














































SimpleTest.waitForExplicitFinish();
var expand = SpecialPowers.Cu.import("resource://gre/modules/PermissionsTable.jsm").expandPermissions;
const permTable = SpecialPowers.Cu.import("resource://gre/modules/PermissionsTable.jsm").PermissionsTable;

const TEST_DOMAIN = "http://example.org";
const SHIM_PATH = "/tests/dom/permission/tests/file_shim.html"
var gContent = document.getElementById('content');


var gCurrentTest = 0;
var gRemainingTests;
var pendingTests = {};

function PermTest(aData) {
  var self = this;
  var skip = aData.skip || false;
  this.step = 0;
  this.data = aData;
  this.isSkip = skip &&
                skip.some(function (el) {
                          return navigator.
                                 userAgent.toLowerCase().
                                 indexOf(el.toLowerCase()) != -1;
                        });

  this.setupParent = false;
  this.perms = expandPermissions(aData.perm);
  this.id = gCurrentTest++;
  this.iframe = null;

  
  pendingTests[this.id] = this;

  this.createFrame = function() {
    if (self.iframe) {
      gContent.removeChild(self.iframe);
    }
    var iframe = document.createElement('iframe');
    iframe.setAttribute('id', 'testframe' + self.step + self.perms)
    iframe.setAttribute('remote', true);
    iframe.src = TEST_DOMAIN + SHIM_PATH;
    iframe.addEventListener('load', function _iframeLoad() {
      iframe.removeEventListener('load', _iframeLoad);

      
      var allow = (self.step == 0 ? false : true);
      self.perms.forEach(function (el) {
        try {
        var res = SpecialPowers.hasPermission(el, SpecialPowers.wrap(iframe)
                                                  .contentDocument);
        is(res, allow, (allow ? "Has " : "Doesn't have ") + el);
        } catch(e) {
          ok(false, "failed " + e);
        }
      });

      var msg = {
        id: self.id,
        step: self.step++,
        testdata: self.data,
      }
      
      iframe.contentWindow.postMessage(msg, "*");
    });

    self.iframe = iframe;
    gContent.appendChild(iframe);
  }

  this.next = function () {
    switch(self.step) {
    case 0:
      self.createFrame();
    break;
    case 1:
      
      addPermissions(self.perms, SpecialPowers.
                                 wrap(self.iframe).
                                 contentDocument,
                     self.createFrame.bind(self));
    break;
    case 2:
      if (self.iframe) {
        gContent.removeChild(self.iframe);
      }
      checkFinish();
    break;
    default:
      ok(false, "Should not be reached");
    break
    }
  }

  this.start = function() {
    
    if (!self.setupParent && self.data.needParentPerm &&
        !SpecialPowers.isMainProcess()) {
      self.setupParent = true;
      addPermissions(self.perms, window.document, self.start.bind(self));
    } else if (self.data.settings && self.data.settings.length) {
      SpecialPowers.pushPrefEnv({'set': self.data.settings.slice(0)},
                                self.next.bind(self));
    } else {
      self.next();
    }
  }
}

function addPermissions(aPerms, aDoc, aCallback) {
  var permList = [];
  aPerms.forEach(function (el) {
    var obj = {'type': el,
               'allow': 1,
               'context': aDoc};
    permList.push(obj);
  });
  SpecialPowers.pushPermissions(permList, aCallback);
}

function expandPermissions(aPerms) {
  var perms = [];
  aPerms.forEach(function(el) {
    var access = permTable[el].access ? "readwrite" : null;
    var expanded = SpecialPowers.unwrap(expand(el, access));
    perms = perms.concat(expanded.slice(0));
  });

  return perms;
}

function msgHandler(evt) {
  var data = evt.data;
  var test = pendingTests[data.id];

  



  if (test.isSkip && test.step == 2) {
    todo(data.result, data.msg);
  } else {
    ok(data.result, data.msg);
  }

  if (test) {
    test.next();
  } else {
    ok(false, "Received unknown id " + data.id);
    checkFinish();
  }
}

function checkFinish() {
  if (--gRemainingTests) {
    gTestRunner.next();
  } else {
    window.removeEventListener('message', msgHandler);
    SimpleTest.finish();
  }
}

function runTest() {
  gRemainingTests = Object.keys(gData).length;

  for (var test in gData) {
    var test = new PermTest(gData[test]);
    test.start();
    yield undefined;
  }
}

var gTestRunner = runTest();

window.addEventListener('load', function() { gTestRunner.next(); }, false);
window.addEventListener('message', msgHandler, false);
