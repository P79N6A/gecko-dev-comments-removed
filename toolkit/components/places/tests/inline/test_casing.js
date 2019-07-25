



add_autocomplete_test([
  "Searching for cased entry",
  "MOZ",
  { autoFilled: "MOZilla.org/", completed: "mozilla.org/"},
  function () {
    addBookmark({ url: "http://mozilla.org/test/" });
  }
]);

add_autocomplete_test([
  "Searching for cased entry",
  "mozilla.org/T",
  { autoFilled: "mozilla.org/Test/", completed: "mozilla.org/test/"},
  function () {
    addBookmark({ url: "http://mozilla.org/test/" });
  }
]);

add_autocomplete_test([
  "Searching for cased entry",
  "mOzilla.org/t",
  { autoFilled: "mOzilla.org/test/", completed: "mozilla.org/Test/"},
  function () {
    addBookmark({ url: "http://mozilla.org/Test/" });
  },
]);
