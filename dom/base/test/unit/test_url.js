function run_test()
{
  var url = new URL('http://www.example.com');
  do_check_eq(url.href, "http://www.example.com/");

  var url2 = new URL('/foobar', url);
  do_check_eq(url2.href, "http://www.example.com/foobar");

  
  
  do_check_false("Blob" in this)

  











}
