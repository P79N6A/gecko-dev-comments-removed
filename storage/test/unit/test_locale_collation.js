















































const DATA_BASENAME = "locale_collation.txt";


var gStrings;


var gLocaleCollation;


var gUtf16Conn;








function cleanupLocaleTests()
{
  print("-- Cleaning up test_locale_collation.js suite.");
  gUtf16Conn.close();
  cleanup();
}







function createUtf16Database()
{
  print("Creating the in-memory UTF-16-encoded database.");
  let conn = getService().openSpecialDatabase("memory");
  conn.executeSimpleSQL("PRAGMA encoding = 'UTF-16'");

  print("Make sure the encoding was set correctly and is now UTF-16.");
  let stmt = conn.createStatement("PRAGMA encoding");
  do_check_true(stmt.executeStep());
  let enc = stmt.getString(0);
  stmt.finalize();

  
  do_check_true(enc === "UTF-16le" || enc === "UTF-16be");

  return conn;
}










function ensureResultsAreCorrect(aActual, aExpected)
{
  print("Actual results:   " + aActual);
  print("Expected results: " + aExpected);

  do_check_eq(aActual.length, aExpected.length);
  for (let i = 0; i < aActual.length; i++)
    do_check_eq(aActual[i], aExpected[i]);
}












function getResults(aCollation, aConn)
{
  let results = [];
  let stmt = aConn.createStatement("SELECT t FROM test " +
                                   "ORDER BY t COLLATE " + aCollation + " ASC");
  while (stmt.executeStep())
    results.push(stmt.row.t);
  stmt.finalize();
  return results;
}









function initTableWithStrings(aStrings, aConn)
{
  print("Initializing test table.");

  aConn.executeSimpleSQL("DROP TABLE IF EXISTS test");
  aConn.createTable("test", "t TEXT");
  let stmt = aConn.createStatement("INSERT INTO test (t) VALUES (:t)");
  aStrings.forEach(function (str) {
    stmt.params.t = str;
    stmt.execute();
    stmt.reset();
  });
  stmt.finalize();
}










function localeCompare(aCollation)
{
  var strength;

  switch (aCollation) {
  case "locale":
    strength = Ci.nsICollation.kCollationCaseInSensitive;
    break;
  case "locale_case_sensitive":
    strength = Ci.nsICollation.kCollationAccentInsenstive;
    break;
  case "locale_accent_sensitive":
    strength = Ci.nsICollation.kCollationCaseInsensitiveAscii;
    break;
  case "locale_case_accent_sensitive":
    strength = Ci.nsICollation.kCollationCaseSensitive;
    break;
  default:
    do_throw("Error in test: unknown collation '" + aCollation + "'");
    break;
  }
  return function (aStr1, aStr2)
         gLocaleCollation.compareString(strength, aStr1, aStr2);
}







function readTestData()
{
  print("Reading in test data.");

  let file = do_get_file(DATA_BASENAME);

  let istream = Cc["@mozilla.org/network/file-input-stream;1"].
                createInstance(Ci.nsIFileInputStream);
  istream.init(file, -1, -1, 0);
  istream.QueryInterface(Components.interfaces.nsILineInputStream);

  let line = {};
  let lines = [];
  while (istream.readLine(line))
    lines.push(line.value); 
  istream.close();

  return lines;
}











function runTest(aCollation, aConn)
{
  ensureResultsAreCorrect(getResults(aCollation, aConn),
                          gStrings.slice(0).sort(localeCompare(aCollation)));
}









function runUtf8Test(aCollation)
{
  runTest(aCollation, getOpenedDatabase());
}









function runUtf16Test(aCollation)
{
  runTest(aCollation, gUtf16Conn);
}




function setup()
{
  print("-- Setting up the test_locale_collation.js suite.");

  gStrings = readTestData();

  initTableWithStrings(gStrings, getOpenedDatabase());

  gUtf16Conn = createUtf16Database();
  initTableWithStrings(gStrings, gUtf16Conn);

  let localeSvc = Cc["@mozilla.org/intl/nslocaleservice;1"].
                  getService(Ci.nsILocaleService);
  let collFact = Cc["@mozilla.org/intl/collation-factory;1"].
                 createInstance(Ci.nsICollationFactory);
  gLocaleCollation = collFact.CreateCollation(localeSvc.getApplicationLocale());
}




let gTests = [
  {
    desc: "Case and accent sensitive UTF-8",
    run:   function () runUtf8Test("locale_case_accent_sensitive")
  },

  {
    desc: "Case sensitive, accent insensitive UTF-8",
    run:   function () runUtf8Test("locale_case_sensitive")
  },

  {
    desc: "Case insensitive, accent sensitive UTF-8",
    run:   function () runUtf8Test("locale_accent_sensitive")
  },

  {
    desc: "Case and accent insensitive UTF-8",
    run:   function () runUtf8Test("locale")
  },

  {
    desc: "Case and accent sensitive UTF-16",
    run:   function () runUtf16Test("locale_case_accent_sensitive")
  },

  {
    desc: "Case sensitive, accent insensitive UTF-16",
    run:   function () runUtf16Test("locale_case_sensitive")
  },

  {
    desc: "Case insensitive, accent sensitive UTF-16",
    run:   function () runUtf16Test("locale_accent_sensitive")
  },

  {
    desc: "Case and accent insensitive UTF-16",
    run:   function () runUtf16Test("locale")
  },
];

function run_test()
{
  setup();
  gTests.forEach(function (test) {
    print("-- Running test: " + test.desc);
    test.run();
  });
}
