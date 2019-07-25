





function run_sql(d, sql) {
  var stmt = d.createStatement(sql)
  stmt.execute()
  stmt.finalize();
}

function new_file(name)
{
  var file = dirSvc.get("ProfD", Ci.nsIFile);
  file.append(name);
  return file;
}

function get_size(name) {
  return new_file(name).fileSize
}

function run_test()
{
  const filename = "chunked.sqlite";
  const CHUNK_SIZE = 512 * 1024;
  var d = getDatabase(new_file(filename));
  d.setGrowthIncrement(CHUNK_SIZE, "");
  run_sql(d, "CREATE TABLE bloat(data varchar)");

  var orig_size = get_size(filename);
  


  const str1024 = new Array(1024).join("T");
  for(var i = 0; i < 32; i++) {
    run_sql(d, "INSERT INTO bloat VALUES('" + str1024 + "')");
    var size = get_size(filename)
    
    do_check_true(size == orig_size || size >= CHUNK_SIZE);
  }
  


  run_sql(d, "DELETE FROM bloat")
  run_sql(d, "VACUUM")
  do_check_true(get_size(filename) >= CHUNK_SIZE)
}

