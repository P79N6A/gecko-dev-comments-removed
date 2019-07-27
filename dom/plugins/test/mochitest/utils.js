function paintCountIs(plugin, expected, msg) {
  var count = plugin.getPaintCount();
  var realExpected = expected;
  var isAsync = SimpleTest.testPluginIsOOP();
  if (isAsync) {
    ++realExpected; 
  } else {
    try {
      if (SpecialPowers.Cc["@mozilla.org/gfx/info;1"].getService(SpecialPowers.Ci.nsIGfxInfo).D2DEnabled) {
        realExpected *= 2;
      }
    } catch (e) {}
  }
  ok(realExpected == count, msg + " (expected " + expected +
     " independent paints, expected " + realExpected + " logged paints, got " +
     count + " actual paints)");
}

function getTestPlugin(pluginName) {
  var ph = SpecialPowers.Cc["@mozilla.org/plugin/host;1"]
                                 .getService(SpecialPowers.Ci.nsIPluginHost);
  var tags = ph.getPluginTags();
  var name = pluginName || "Test Plug-in";
  for (var tag of tags) {
    if (tag.name == name) {
      return tag;
    }
  }

  ok(false, "Could not find plugin tag with plugin name '" + name + "'");
  return null;
}




function setTestPluginEnabledState(newEnabledState, pluginName) {
  var oldEnabledState = SpecialPowers.setTestPluginEnabledState(newEnabledState, pluginName);
  if (!oldEnabledState) {
    ok(false, "Cannot find plugin '" + plugin + "'");
    return;
  }
  var plugin = getTestPlugin(pluginName);
  while (plugin.enabledState != newEnabledState) {
    
    
    SpecialPowers.Services.tm.currentThread.processNextEvent(true);
  }
  SimpleTest.registerCleanupFunction(function() {
    SpecialPowers.setTestPluginEnabledState(oldEnabledState, pluginName);
  });
}

function crashAndGetCrashServiceRecord(crashMethodName, callback) {
  var crashMan =
    SpecialPowers.Cu.import("resource://gre/modules/Services.jsm").
    Services.crashmanager;

  
  info("Waiting for pruneOldCrashes");
  var future = new Date(Date.now() + 1000 * 60 * 60 * 24);
  crashMan.pruneOldCrashes(future).then(function () {

    var iframe = document.getElementById("iframe1");
    var p = iframe.contentDocument.getElementById("plugin1");

    var crashDateMS = Date.now();
    try {
      p[crashMethodName]();
      ok(false, "p." + crashMethodName + "() should throw an exception");
    }
    catch (e) {
      ok(true, "p." + crashMethodName + "() should throw an exception");
    }

    
    
    function tryGetCrash() {
      info("Waiting for getCrashes");
      crashMan.getCrashes().then(SpecialPowers.wrapCallback(function (crashes) {
        if (crashes.length) {
          is(crashes.length, 1, "There should be only one record");
          var crash = SpecialPowers.wrap(crashes[0]);
          ok(!!crash.id, "Record should have an ID");
          ok(!!crash.crashDate, "Record should have a crash date");
          var dateMS = crash.crashDate.valueOf();
          var twoMin = 1000 * 60 * 2;
          ok(crashDateMS - twoMin <= dateMS &&
             dateMS <= crashDateMS + twoMin,
             "Record's crash date should be nowish: " +
             "now=" + crashDateMS + " recordDate=" + dateMS);
          callback(crashMan, crash);
        }
        else {
          setTimeout(tryGetCrash, 1000);
        }
      }), function (err) {
        ok(false, "Error getting crashes: " + err);
        SimpleTest.finish();
      });
    }
    setTimeout(tryGetCrash, 1000);

  }, function () {
    ok(false, "pruneOldCrashes error");
    SimpleTest.finish();
  });
}
