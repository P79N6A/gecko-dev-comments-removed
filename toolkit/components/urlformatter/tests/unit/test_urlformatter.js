


































function run_test() {
  var formatter = Cc["@mozilla.org/toolkit/URLFormatterService;1"].
                  getService(Ci.nsIURLFormatter);
  var locale = Cc["@mozilla.org/chrome/chrome-registry;1"].
               getService(Ci.nsIXULChromeRegistry).
               getSelectedLocale('global');
  var prefs = Cc['@mozilla.org/preferences-service;1'].
              getService(Ci.nsIPrefBranch);

  var upperUrlRaw = "http://%LOCALE%.%VENDOR%.foo/?name=%NAME%&id=%ID%&version=%VERSION%&platversion=%PLATFORMVERSION%&abid=%APPBUILDID%&pbid=%PLATFORMBUILDID%&app=%APP%&os=%OS%&abi=%XPCOMABI%";
  var lowerUrlRaw = "http://%locale%.%vendor%.foo/?name=%name%&id=%id%&version=%version%&platversion=%platformversion%&abid=%appbuildid%&pbid=%platformbuildid%&app=%app%&os=%os%&abi=%xpcomabi%";
  
  var ulUrlRef = "http://" + locale + ".Mozilla.foo/?name=Url Formatter Test&id=urlformattertest@test.mozilla.org&version=1&platversion=2.0&abid=2007122405&pbid=2007122406&app=urlformatter test&os=XPCShell&abi=noarch-spidermonkey";
  var multiUrl = "http://%VENDOR%.%VENDOR%.%NAME%.%VENDOR%.%NAME%";
  var multiUrlRef = "http://Mozilla.Mozilla.Url Formatter Test.Mozilla.Url Formatter Test";
  var encodedUrl = "https://%LOCALE%.%VENDOR%.foo/?q=%E3%82%BF%E3%83%96&app=%NAME%&ver=%PLATFORMVERSION%";
  var encodedUrlRef = "https://" + locale + ".Mozilla.foo/?q=%E3%82%BF%E3%83%96&app=Url Formatter Test&ver=2.0";

  var pref = "xpcshell.urlformatter.test";
  var str = Cc["@mozilla.org/supports-string;1"].
            createInstance(Ci.nsISupportsString);
  str.data = upperUrlRaw;
  prefs.setComplexValue(pref, Ci.nsISupportsString, str);

  do_check_eq(formatter.formatURL(upperUrlRaw), ulUrlRef);
  do_check_eq(formatter.formatURLPref(pref), ulUrlRef);
  
  do_check_neq(formatter.formatURL(lowerUrlRaw), ulUrlRef);
  do_check_eq(formatter.formatURL(multiUrl), multiUrlRef);
  
  do_check_eq(formatter.formatURL(encodedUrl), encodedUrlRef);
}
