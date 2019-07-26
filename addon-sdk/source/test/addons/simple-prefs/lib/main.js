


'use strict';

const { Cu } = require('chrome');
const sp = require('sdk/simple-prefs');
const app = require('sdk/system/xul-app');
const self = require('sdk/self');
const tabs = require('sdk/tabs');
const { preferencesBranch } = require('sdk/self');

const { AddonManager } = Cu.import('resource://gre/modules/AddonManager.jsm', {});

exports.testDefaultValues = function (assert) {
  assert.equal(sp.prefs.myHiddenInt, 5, 'myHiddenInt default is 5');
  assert.equal(sp.prefs.myInteger, 8, 'myInteger default is 8');
  assert.equal(sp.prefs.somePreference, 'TEST', 'somePreference default is correct');
}

exports.testOptionsType = function(assert, done) {
  AddonManager.getAddonByID(self.id, function(aAddon) {
    assert.equal(aAddon.optionsType, AddonManager.OPTIONS_TYPE_INLINE, 'options type is inline');
    done();
  });
}

exports.testButton = function(assert, done) {
  tabs.open({
    url: 'about:addons',
    onReady: function(tab) {
      sp.once('sayHello', function() {
        assert.pass('The button was pressed!');
        tab.close(done)
      });

      tab.attach({
        contentScriptWhen: 'end',
        contentScript: 'function onLoad() {\n' +
                         'unsafeWindow.removeEventListener("load", onLoad, false);\n' +
                         'AddonManager.getAddonByID("' + self.id + '", function(aAddon) {\n' +
                           'unsafeWindow.gViewController.viewObjects.detail.node.addEventListener("ViewChanged", function whenViewChanges() {\n' +
                             'unsafeWindow.gViewController.viewObjects.detail.node.removeEventListener("ViewChanged", whenViewChanges, false);\n' +
                             'setTimeout(function() {\n' + 
                               'unsafeWindow.document.querySelector("button[label=\'Click me!\']").click()\n' +
                             '}, 250);\n' +
                           '}, false);\n' +
                           'unsafeWindow.gViewController.commands.cmd_showItemDetails.doCommand(aAddon, true);\n' +
                         '});\n' +
                       '}\n' +
                       
                       'if (document.readyState == "complete") {\n' +
                         'onLoad()\n' +
                       '} else {\n' +
                         'unsafeWindow.addEventListener("load", onLoad, false);\n' +
                       '}\n',
      });
    }
  });
}

if (app.is('Firefox')) {
  exports.testAOM = function(assert, done) {
      tabs.open({
      	url: 'about:addons',
      	onReady: function(tab) {
          tab.attach({
            contentScriptWhen: 'end',
          	contentScript: 'function onLoad() {\n' +
                             'unsafeWindow.removeEventListener("load", onLoad, false);\n' +
                             'AddonManager.getAddonByID("' + self.id + '", function(aAddon) {\n' +
                               'unsafeWindow.gViewController.viewObjects.detail.node.addEventListener("ViewChanged", function whenViewChanges() {\n' +
                                 'unsafeWindow.gViewController.viewObjects.detail.node.removeEventListener("ViewChanged", whenViewChanges, false);\n' +
                                 'setTimeout(function() {\n' + 
                                     'self.postMessage({\n' +
                                       'someCount: unsafeWindow.document.querySelectorAll("setting[title=\'some-title\']").length,\n' +
                                       'somePreference: getAttributes(unsafeWindow.document.querySelector("setting[title=\'some-title\']")),\n' +
                                       'myInteger: getAttributes(unsafeWindow.document.querySelector("setting[title=\'my-int\']")),\n' +
                                       'myHiddenInt: getAttributes(unsafeWindow.document.querySelector("setting[title=\'hidden-int\']")),\n' +
                                       'sayHello: getAttributes(unsafeWindow.document.querySelector("button[label=\'Click me!\']"))\n' +
                                     '});\n' +
                                 '}, 250);\n' +
                               '}, false);\n' +
                               'unsafeWindow.gViewController.commands.cmd_showItemDetails.doCommand(aAddon, true);\n' +
                             '});\n' +
                             'function getAttributes(ele) {\n' +
                               'if (!ele) return {};\n' +
                               'return {\n' +
                                 'pref: ele.getAttribute("pref"),\n' +
                                 'type: ele.getAttribute("type"),\n' +
                                 'title: ele.getAttribute("title"),\n' +
                                 'desc: ele.getAttribute("desc"),\n' +
                                 '"data-jetpack-id": ele.getAttribute(\'data-jetpack-id\')\n' +
                               '}\n' +
                             '}\n' +
                           '}\n' +
                           
                           'if (document.readyState == "complete") {\n' +
                             'onLoad()\n' +
                           '} else {\n' +
                             'unsafeWindow.addEventListener("load", onLoad, false);\n' +
                           '}\n',
            onMessage: function(msg) {
              
              assert.equal(msg.someCount, 1, 'there is exactly one <setting> node for somePreference');

              
              assert.equal(msg.somePreference.type, 'string', 'some pref is a string');
              assert.equal(msg.somePreference.pref, 'extensions.'+self.id+'.somePreference', 'somePreference path is correct');
              assert.equal(msg.somePreference.title, 'some-title', 'somePreference title is correct');
              assert.equal(msg.somePreference.desc, 'Some short description for the preference', 'somePreference description is correct');
              assert.equal(msg.somePreference['data-jetpack-id'], self.id, 'data-jetpack-id attribute value is correct');

              
              assert.equal(msg.myInteger.type, 'integer', 'myInteger is a int');
              assert.equal(msg.myInteger.pref, 'extensions.'+self.id+'.myInteger', 'extensions.test-simple-prefs.myInteger');
              assert.equal(msg.myInteger.title, 'my-int', 'myInteger title is correct');
              assert.equal(msg.myInteger.desc, 'How many of them we have.', 'myInteger desc is correct');
              assert.equal(msg.myInteger['data-jetpack-id'], self.id, 'data-jetpack-id attribute value is correct');

              
              assert.equal(msg.myHiddenInt.type, undefined, 'myHiddenInt was not displayed');
              assert.equal(msg.myHiddenInt.pref, undefined, 'myHiddenInt was not displayed');
              assert.equal(msg.myHiddenInt.title, undefined, 'myHiddenInt was not displayed');
              assert.equal(msg.myHiddenInt.desc, undefined, 'myHiddenInt was not displayed');

              
              assert.equal(msg.sayHello['data-jetpack-id'], self.id, 'data-jetpack-id attribute value is correct');

              tab.close(done);
            }
          });
      	}
      });
  }

  
  
  exports.testAgainstDocCaching = exports.testAOM;

}

exports.testDefaultPreferencesBranch = function(assert) {
  assert.equal(preferencesBranch, self.id, 'preferencesBranch default the same as self.id');
}

require('sdk/test/runner').runTestsFromModule(module);
