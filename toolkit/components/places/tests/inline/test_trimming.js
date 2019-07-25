



add_autocomplete_test([
  "Searching for untrimmed https://www entry",
  "mo",
  { autoFilled: "mozilla.org/", completed: "https://www.mozilla.org/" },
  function () {
    addBookmark({ url: "https://www.mozilla.org/test/" });
  },
]);

add_autocomplete_test([
  "Searching for untrimmed https://www entry with path",
  "mozilla.org/t",
  { autoFilled: "mozilla.org/test/", completed: "https://www.mozilla.org/test/" },
  function () {
    addBookmark({ url: "https://www.mozilla.org/test/" });
  },
]);

add_autocomplete_test([
  "Searching for untrimmed https:// entry",
  "mo",
  { autoFilled: "mozilla.org/", completed: "https://mozilla.org/" },
  function () {
    addBookmark({ url: "https://mozilla.org/test/" });
  },
]);

add_autocomplete_test([
  "Searching for untrimmed https:// entry with path",
  "mozilla.org/t",
  { autoFilled: "mozilla.org/test/", completed: "https://mozilla.org/test/" },
  function () {
    addBookmark({ url: "https://mozilla.org/test/" });
  },
]);

add_autocomplete_test([
  "Searching for untrimmed http://www entry",
  "mo",
  { autoFilled: "mozilla.org/", completed: "www.mozilla.org/" },
  function () {
    addBookmark({ url: "http://www.mozilla.org/test/" });
  },
]);

add_autocomplete_test([
  "Searching for untrimmed http://www entry with path",
  "mozilla.org/t",
  { autoFilled: "mozilla.org/test/", completed: "http://www.mozilla.org/test/" },
  function () {
    addBookmark({ url: "http://www.mozilla.org/test/" });
  },
]);

add_autocomplete_test([
  "Searching for untrimmed ftp:// entry",
  "mo",
  { autoFilled: "mozilla.org/", completed: "ftp://mozilla.org/" },
  function () {
    addBookmark({ url: "ftp://mozilla.org/test/" });
  },
]);

add_autocomplete_test([
  "Searching for untrimmed ftp:// entry with path",
  "mozilla.org/t",
  { autoFilled: "mozilla.org/test/", completed: "ftp://mozilla.org/test/" },
  function () {
    addBookmark({ url: "ftp://mozilla.org/test/" });
  },
]);

add_autocomplete_test([
  "Ensuring correct priority 1",
  "mo",
  { autoFilled: "mozilla.org/", completed: "https://www.mozilla.org/" },
  function () {
    addBookmark({ url: "https://www.mozilla.org/test/" });
    addBookmark({ url: "https://mozilla.org/test/" });
    addBookmark({ url: "ftp://mozilla.org/test/" });
    addBookmark({ url: "http://www.mozilla.org/test/" });
    addBookmark({ url: "http://mozilla.org/test/" });
  },
]);

add_autocomplete_test([
  "Ensuring correct priority 2",
  "mo",
  { autoFilled: "mozilla.org/", completed: "https://mozilla.org/" },
  function () {
    addBookmark({ url: "https://mozilla.org/test/" });
    addBookmark({ url: "ftp://mozilla.org/test/" });
    addBookmark({ url: "http://www.mozilla.org/test/" });
    addBookmark({ url: "http://mozilla.org/test/" });
  },
]);

add_autocomplete_test([
  "Ensuring correct priority 3",
  "mo",
  { autoFilled: "mozilla.org/", completed: "ftp://mozilla.org/" },
  function () {
    addBookmark({ url: "ftp://mozilla.org/test/" });
    addBookmark({ url: "http://www.mozilla.org/test/" });
    addBookmark({ url: "http://mozilla.org/test/" });
  },
]);

add_autocomplete_test([
  "Ensuring correct priority 4",
  "mo",
  { autoFilled: "mozilla.org/", completed: "www.mozilla.org/" },
  function () {
    addBookmark({ url: "http://www.mozilla.org/test/" });
    addBookmark({ url: "http://mozilla.org/test/" });
  },
]);

add_autocomplete_test([
  "Ensuring longer domain can't match",
  "mo",
  { autoFilled: "mozilla.co/", completed: "mozilla.co/" },
  function () {
    
    
    addBookmark({ url: "https://mozilla.com/" });
    addBookmark({ url: "http://mozilla.co/" });
    addBookmark({ url: "http://mozilla.co/" });
  },
]);

add_autocomplete_test([
  "Searching for URL with characters that are normally escaped",
  "https://www.mozilla.org/啊-test",
  { autoFilled: "https://www.mozilla.org/啊-test", completed: "https://www.mozilla.org/啊-test" },
  function () {
    addVisits({ uri: NetUtil.newURI("https://www.mozilla.org/啊-test"),
                transition: TRANSITION_TYPED });
  },
]);
