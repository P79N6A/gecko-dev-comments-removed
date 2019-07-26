





function run_test()
{
  run_next_test();
}

add_task(function test_execute()
{
  var testURI = uri("wyciwyg://nodontjudgeabookbyitscover");

  try
  {
    yield promiseAddVisits(testURI);
    do_throw("Should have generated an exception.");
  } catch (ex if ex && ex.result == Cr.NS_ERROR_ILLEGAL_VALUE) {
    
  }
});
