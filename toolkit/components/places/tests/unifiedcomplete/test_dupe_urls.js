




add_task(function* test_dupe_urls() {
  do_log_info("Searching for urls with dupes should only show one");
  yield promiseAddVisits({ uri: NetUtil.newURI("http://mozilla.org/"),
                           transition: TRANSITION_TYPED },
                         { uri: NetUtil.newURI("http://mozilla.org/?") });
  yield check_autocomplete({
    search: "moz",
    autofilled: "mozilla.org/",
    completed:  "mozilla.org/",
    matches: [ { uri: NetUtil.newURI("http://mozilla.org/"),
                 title: "mozilla.org" } ]
  });
  yield cleanup();
});
