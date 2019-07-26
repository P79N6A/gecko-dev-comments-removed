


const URL = "http://mochi.test:8888/?t=" + Date.now();
const URL1 = URL + "#1";
const URL2 = URL + "#2";
const URL3 = URL + "#3";

XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
  "resource://gre/modules/FileUtils.jsm");

let tmp = {};
Cc["@mozilla.org/moz/jssubscript-loader;1"]
  .getService(Ci.mozIJSSubScriptLoader)
  .loadSubScript("resource://gre/modules/PageThumbs.jsm", tmp);

const {EXPIRATION_MIN_CHUNK_SIZE, PageThumbsExpiration} = tmp;

function runTests() {
  
  createDummyThumbnail(URL1);
  ok(thumbnailExists(URL1), "first thumbnail created");

  createDummyThumbnail(URL2);
  ok(thumbnailExists(URL2), "second thumbnail created");

  createDummyThumbnail(URL3);
  ok(thumbnailExists(URL3), "third thumbnail created");

  
  yield expireThumbnails([URL1, URL2]);
  ok(thumbnailExists(URL1), "first thumbnail still exists");
  ok(thumbnailExists(URL2), "second thumbnail still exists");
  ok(!thumbnailExists(URL3), "third thumbnail has been removed");

  
  yield expireThumbnails([URL1]);
  ok(thumbnailExists(URL1), "first thumbnail still exists");
  ok(!thumbnailExists(URL2), "second thumbnail has been removed");

  
  yield expireThumbnails([]);
  ok(!thumbnailExists(URL1), "all thumbnails have been removed");

  
  let urls = [];
  for (let i = 0; i < EXPIRATION_MIN_CHUNK_SIZE + 10; i++) {
    urls.push(URL + "#dummy" + i);
  }

  urls.forEach(createDummyThumbnail);
  ok(urls.every(thumbnailExists), "all dummy thumbnails created");

  
  let dontExpireDummyURLs = function (cb) cb(urls);
  PageThumbs.addExpirationFilter(dontExpireDummyURLs);

  registerCleanupFunction(function () {
    PageThumbs.removeExpirationFilter(dontExpireDummyURLs);
  });

  
  yield expireThumbnails([]);
  let remainingURLs = [u for (u of urls) if (thumbnailExists(u))];
  is(remainingURLs.length, 10, "10 dummy thumbnails remaining");

  
  yield expireThumbnails([]);
  remainingURLs = [u for (u of remainingURLs) if (thumbnailExists(u))];
  is(remainingURLs.length, 0, "no dummy thumbnails remaining");
}

function createDummyThumbnail(aURL) {
  let file = PageThumbsStorage.getFileForURL(aURL);
  let fos = FileUtils.openSafeFileOutputStream(file);

  let data = "dummy";
  fos.write(data, data.length);
  FileUtils.closeSafeFileOutputStream(fos);
}

function expireThumbnails(aKeep) {
  PageThumbsExpiration.expireThumbnails(aKeep, function () {
    executeSoon(next);
  });
}
