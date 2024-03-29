



add_autocomplete_test([
  "Searching for non-keyworded entry should autoFill it",
  "moz",
  "mozilla.org/",
  function* () {
    yield addBookmark({ url: "http://mozilla.org/test/" });
  }
]);

add_autocomplete_test([
  "Searching for keyworded entry should not autoFill it",
  "moz",
  "moz",
  function* () {
    yield addBookmark({ url: "http://mozilla.org/test/", keyword: "moz" });
  }
]);

add_autocomplete_test([
  "Searching for more than keyworded entry should autoFill it",
  "mozi",
  "mozilla.org/",
  function* () {
    yield addBookmark({ url: "http://mozilla.org/test/", keyword: "moz" });
  }
]);

add_autocomplete_test([
  "Searching for less than keyworded entry should autoFill it",
  "mo",
  "mozilla.org/",
  function* () {
    yield addBookmark({ url: "http://mozilla.org/test/", keyword: "moz" });
  }
]);

add_autocomplete_test([
  "Searching for keyworded entry is case-insensitive",
  "MoZ",
  "MoZ",
  function* () {
    yield addBookmark({ url: "http://mozilla.org/test/", keyword: "moz" });
  }
]);
