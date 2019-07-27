





let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/Services.jsm");

let NS_ERROR_INVALID_ARG = Components.results.NS_ERROR_INVALID_ARG;

function do_check_throws(f, result, stack)
{
  if (!stack) {
    try {
      
      stack = Components.stack.caller;
    } catch (e) {
    }
  }

  try {
    f();
  } catch (exc) {
    do_check_eq(exc.result, result);
    return;
  }
  do_throw("expected " + result + " exception, none thrown", stack);
}

function run_test() {
  let util = Cc["@mozilla.org/thirdpartyutil;1"].getService(Ci.mozIThirdPartyUtil);

  
  
  let spec1 = "http://foo.com/foo.html";
  let spec2 = "http://bar.com/bar.html";
  let uri1 = NetUtil.newURI(spec1);
  let uri2 = NetUtil.newURI(spec2);
  let channel1 = NetUtil.newChannel2(uri1,
                                     null,
                                     null,
                                     null,      
                                     Services.scriptSecurityManager.getSystemPrincipal(),
                                     null,      
                                     Ci.nsILoadInfo.SEC_NORMAL,
                                     Ci.nsIContentPolicy.TYPE_OTHER);
  let channel2 = NetUtil.newChannel2(uri2,
                                     null,
                                     null,
                                     null,      
                                     Services.scriptSecurityManager.getSystemPrincipal(),
                                     null,      
                                     Ci.nsILoadInfo.SEC_NORMAL,
                                     Ci.nsIContentPolicy.TYPE_OTHER);

  
  let filespec1 = "file://foo.txt";
  let filespec2 = "file://bar.txt";
  let fileuri1 = NetUtil.newURI(filespec1);
  let fileuri2 = NetUtil.newURI(filespec2);
  let filechannel1 = NetUtil.newChannel2(fileuri1,
                                         null,
                                         null,
                                         null,      
                                         Services.scriptSecurityManager.getSystemPrincipal(),
                                         null,      
                                         Ci.nsILoadInfo.SEC_NORMAL,
                                         Ci.nsIContentPolicy.TYPE_OTHER);
  let filechannel2 = NetUtil.newChannel2(fileuri2,
                                         null,
                                         null,
                                         null,      
                                         Services.scriptSecurityManager.getSystemPrincipal(),
                                         null,      
                                         Ci.nsILoadInfo.SEC_NORMAL,
                                         Ci.nsIContentPolicy.TYPE_OTHER);

  
  do_check_false(util.isThirdPartyURI(uri1, uri1));
  do_check_true(util.isThirdPartyURI(uri1, uri2));
  do_check_true(util.isThirdPartyURI(uri2, uri1));
  do_check_false(util.isThirdPartyURI(fileuri1, fileuri1));
  do_check_false(util.isThirdPartyURI(fileuri1, fileuri2));
  do_check_true(util.isThirdPartyURI(uri1, fileuri1));
  do_check_throws(function() { util.isThirdPartyURI(uri1, null); },
    NS_ERROR_INVALID_ARG);
  do_check_throws(function() { util.isThirdPartyURI(null, uri1); },
    NS_ERROR_INVALID_ARG);
  do_check_throws(function() { util.isThirdPartyURI(null, null); },
    NS_ERROR_INVALID_ARG);

  
  

  
  
  do_check_throws(function() { util.isThirdPartyChannel(null); },
    NS_ERROR_INVALID_ARG);
  do_check_throws(function() { util.isThirdPartyChannel(channel1); },
    NS_ERROR_INVALID_ARG);
  do_check_throws(function() { util.isThirdPartyChannel(channel1, uri1); },
    NS_ERROR_INVALID_ARG);
  do_check_true(util.isThirdPartyChannel(channel1, uri2));
  let httpchannel1 = channel1.QueryInterface(Ci.nsIHttpChannelInternal);
  httpchannel1.forceAllowThirdPartyCookie = true;
  do_check_false(util.isThirdPartyChannel(channel1));
  do_check_false(util.isThirdPartyChannel(channel1, uri1));
  do_check_true(util.isThirdPartyChannel(channel1, uri2));
}

