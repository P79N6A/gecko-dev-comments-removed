




































const LATIN1_AE = "\xc6";
const LATIN1_ae = "\xe6"; 

function setup()
{
  getOpenedDatabase().createTable("t1", "x TEXT");

  var stmt = createStatement("INSERT INTO t1 (x) VALUES ('foo/bar_baz%20cheese')");
  stmt.execute();

  stmt = createStatement("INSERT INTO t1 (x) VALUES (?1)");
  
  stmt.bindStringParameter(0, "foo%20" + LATIN1_ae + "/_bar");
  stmt.execute();
}
    
function test_escape_for_like_ascii()
{
  var stmt = createStatement("SELECT x FROM t1 WHERE x LIKE ?1 ESCAPE '/'");
  var paramForLike = stmt.escapeStringForLIKE("oo/bar_baz%20chees", '/');
  
  do_check_eq(paramForLike, "oo//bar/_baz/%20chees");
  
  stmt.bindStringParameter(0, "%" + paramForLike + "%"); 
  stmt.executeStep();
  do_check_eq("foo/bar_baz%20cheese", stmt.getString(0));
}

function test_escape_for_like_non_ascii()
{
  var stmt = createStatement("SELECT x FROM t1 WHERE x LIKE ?1 ESCAPE '/'");
  var paramForLike = stmt.escapeStringForLIKE("oo%20" + LATIN1_AE + "/_ba", '/');
  
  do_check_eq(paramForLike, "oo/%20" + LATIN1_AE + "///_ba");
  
  stmt.bindStringParameter(0, "%" + paramForLike + "%");
  stmt.executeStep();
  do_check_eq("foo%20" + LATIN1_ae + "/_bar", stmt.getString(0));
}

var tests = [test_escape_for_like_ascii, test_escape_for_like_non_ascii];

function run_test()
{
  setup();

  for (var i = 0; i < tests.length; i++)
    tests[i]();
    
  cleanup();
}
