


const URL = "http://mochi.test:8888/?t=" + Date.now();
const URL1 = URL + "#1";
const URL2 = URL + "#2";
const URL3 = URL + "#3";

let tmp = {};
Cc["@mozilla.org/moz/jssubscript-loader;1"]
  .getService(Ci.mozIJSSubScriptLoader)
  .loadSubScript("resource://gre/modules/PageThumbs.jsm", tmp);

const {EXPIRATION_MIN_CHUNK_SIZE, PageThumbsExpiration} = tmp;

function runTests() {
  
  let dummyURLs = [];
  for (let i = 0; i < EXPIRATION_MIN_CHUNK_SIZE + 10; i++) {
    dummyURLs.push(URL + "#dummy" + i);
  }

  
  dontExpireThumbnailURLs([URL1, URL2, URL3].concat(dummyURLs));

  
  yield createDummyThumbnail(URL1);
  ok(thumbnailExists(URL1), "first thumbnail created");

  yield createDummyThumbnail(URL2);
  ok(thumbnailExists(URL2), "second thumbnail created");

  yield createDummyThumbnail(URL3);
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

  
  for (let url of dummyURLs) {
    yield createDummyThumbnail(url);
  }

  ok(dummyURLs.every(thumbnailExists), "all dummy thumbnails created");

  
  yield expireThumbnails([]);
  let remainingURLs = [u for (u of dummyURLs) if (thumbnailExists(u))];
  is(remainingURLs.length, 10, "10 dummy thumbnails remaining");

  
  yield expireThumbnails([]);
  remainingURLs = [u for (u of remainingURLs) if (thumbnailExists(u))];
  is(remainingURLs.length, 0, "no dummy thumbnails remaining");
}

function createDummyThumbnail(aURL) {
  info("Creating dummy thumbnail for " + aURL);
  let dummy = new Uint8Array(10);
  for (let i = 0; i < 10; ++i) {
    dummy[i] = i;
  }
  PageThumbsStorage.writeData(aURL, dummy).then(
    function onSuccess() {
      info("createDummyThumbnail succeeded");
      executeSoon(next);
    },
    function onFailure(error) {
      ok(false, "createDummyThumbnail failed " + error);
    }
  );
}

function expireThumbnails(aKeep) {
  PageThumbsExpiration.expireThumbnails(aKeep).then(
    function onSuccess() {
      info("expireThumbnails succeeded");
      executeSoon(next);
    },
    function onFailure(error) {
      ok(false, "expireThumbnails failed " + error);
    }
  );
}
