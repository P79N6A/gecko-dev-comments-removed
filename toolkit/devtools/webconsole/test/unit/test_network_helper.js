


"use strict";
const Cu = Components.utils;
const { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});

Object.defineProperty(this, "NetworkHelper", {
  get: function() {
    return devtools.require("devtools/toolkit/webconsole/network-helper");
  },
  configurable: true,
  writeable: false,
  enumerable: true
});

function run_test() {
  test_isTextMimeType();
}

function test_isTextMimeType () {
  do_check_eq(NetworkHelper.isTextMimeType("text/plain"), true);
  do_check_eq(NetworkHelper.isTextMimeType("application/javascript"), true);
  do_check_eq(NetworkHelper.isTextMimeType("application/json"), true);
  do_check_eq(NetworkHelper.isTextMimeType("text/css"), true);
  do_check_eq(NetworkHelper.isTextMimeType("text/html"), true);
  do_check_eq(NetworkHelper.isTextMimeType("image/svg+xml"), true);
  do_check_eq(NetworkHelper.isTextMimeType("application/xml"), true);

  
  do_check_eq(NetworkHelper.isTextMimeType("application/vnd.tent.posts-feed.v0+json"), true);
  do_check_eq(NetworkHelper.isTextMimeType("application/vnd.tent.posts-feed.v0-json"), true);
  
  do_check_eq(NetworkHelper.isTextMimeType("application/vnd.tent.posts-feed.v0+xml"), true);
  do_check_eq(NetworkHelper.isTextMimeType("application/vnd.tent.posts-feed.v0-xml"), false);
  
  do_check_eq(NetworkHelper.isTextMimeType("application/vnd.BIG-CORP+json"), true);
  
  do_check_eq(NetworkHelper.isTextMimeType("image/png"), false);
  
  do_check_eq(NetworkHelper.isTextMimeType("application/foo-+json"), false);
  do_check_eq(NetworkHelper.isTextMimeType("application/-foo+json"), false);
  do_check_eq(NetworkHelper.isTextMimeType("application/foo--bar+json"), false);

  
  do_check_eq(NetworkHelper.isTextMimeType("application/vnd.google.safebrowsing-chunk"), false);
}
