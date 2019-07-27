






add_task(function* test_not_autofill_ws_1() {
  do_log_info("Do not autofill whitespaced entry 1");
  yield promiseAddVisits({ uri: NetUtil.newURI("http://mozilla.org/link/"),
                           transition: TRANSITION_TYPED });
  yield check_autocomplete({
    search: "mozilla.org ",
    autofilled: "mozilla.org ",
    completed: "mozilla.org "
  });
  yield cleanup();
});

add_task(function* test_not_autofill_ws_2() {
  do_log_info("Do not autofill whitespaced entry 2");
  yield promiseAddVisits({ uri: NetUtil.newURI("http://mozilla.org/link/"),
                           transition: TRANSITION_TYPED });
  yield check_autocomplete({
    search: "mozilla.org/ ",
    autofilled: "mozilla.org/ ",
    completed: "mozilla.org/ "
  });
  yield cleanup();
});

add_task(function* test_not_autofill_ws_3() {
  do_log_info("Do not autofill whitespaced entry 3");
  yield promiseAddVisits({ uri: NetUtil.newURI("http://mozilla.org/link/"),
                           transition: TRANSITION_TYPED });
  yield check_autocomplete({
    search: "mozilla.org/link ",
    autofilled: "mozilla.org/link ",
    completed: "mozilla.org/link "
  });
  yield cleanup();
});

add_task(function* test_not_autofill_ws_4() {
  do_log_info("Do not autofill whitespaced entry 4");
  yield promiseAddVisits({ uri: NetUtil.newURI("http://mozilla.org/link/"),
                           transition: TRANSITION_TYPED });
  yield check_autocomplete({
    search: "mozilla.org/link/ ",
    autofilled: "mozilla.org/link/ ",
    completed: "mozilla.org/link/ "
  });
  yield cleanup();
});


add_task(function* test_not_autofill_ws_5() {
  do_log_info("Do not autofill whitespaced entry 5");
  yield promiseAddVisits({ uri: NetUtil.newURI("http://mozilla.org/link/"),
                           transition: TRANSITION_TYPED });
  yield check_autocomplete({
    search: "moz illa ",
    autofilled: "moz illa ",
    completed: "moz illa "
  });
  yield cleanup();
});

add_task(function* test_not_autofill_ws_6() {
  do_log_info("Do not autofill whitespaced entry 6");
  yield promiseAddVisits({ uri: NetUtil.newURI("http://mozilla.org/link/"),
                           transition: TRANSITION_TYPED });
  yield check_autocomplete({
    search: " mozilla",
    autofilled: " mozilla",
    completed: " mozilla"
  });
  yield cleanup();
});
