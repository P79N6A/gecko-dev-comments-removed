try {
  Cu.import("resource://weave/log4moz.js");
  Cu.import("resource://weave/util.js");
  Cu.import("resource://weave/auth.js");
  Cu.import("resource://weave/identity.js");
  Cu.import("resource://weave/resource.js");
  Cu.import("resource://weave/base_records/wbo.js");
  Cu.import("resource://weave/base_records/collection.js");
} catch (e) { do_throw(e); }

function record_handler(metadata, response) {
  let obj = {id: "asdf-1234-asdf-1234",
             modified: 2454725.98283,
             payload: JSON.stringify({cheese: "roquefort"})};
  return httpd_basic_auth_handler(JSON.stringify(obj), metadata, response);
}

function record_handler2(metadata, response) {
  let obj = {id: "record2",
             modified: 2454725.98284,
             payload: JSON.stringify({cheese: "gruyere"})};
  return httpd_basic_auth_handler(JSON.stringify(obj), metadata, response);
}

function coll_handler(metadata, response) {
  let obj = [{id: "record2",
              modified: 2454725.98284,
              payload: JSON.stringify({cheese: "gruyere"})}];
  return httpd_basic_auth_handler(JSON.stringify(obj), metadata, response);
}

function run_test() {
  let server;

  try {
    let log = Log4Moz.repository.getLogger('Test');
    Log4Moz.repository.rootLogger.addAppender(new Log4Moz.DumpAppender());

    log.info("Setting up server and authenticator");

    server = httpd_setup({"/record": record_handler,
                          "/record2": record_handler2,
                          "/coll": coll_handler});

    let auth = new BasicAuthenticator(new Identity("secret", "guest", "guest"));
    Auth.defaultAuthenticator = auth;

    log.info("Getting a WBO record");

    let res = new Resource("http://localhost:8080/record");
    res.get();

    let rec = new WBORecord();
    rec.deserialize(res.data);
    do_check_eq(rec.id, "asdf-1234-asdf-1234"); 

    rec.uri = res.uri;
    do_check_eq(rec.id, "record"); 

    do_check_eq(rec.modified, 2454725.98283);
    do_check_eq(typeof(rec.payload), "object");
    do_check_eq(rec.payload.cheese, "roquefort");
    do_check_eq(res.lastChannel.responseStatus, 200);

    log.info("Getting a WBO record using the record manager");

    let rec2 = Records.get("http://localhost:8080/record2");
    do_check_eq(rec2.id, "record2");
    do_check_eq(rec2.modified, 2454725.98284);
    do_check_eq(typeof(rec2.payload), "object");
    do_check_eq(rec2.payload.cheese, "gruyere");
    do_check_eq(Records.lastResource.lastChannel.responseStatus, 200);

    log.info("Using a collection to get a record");

    let coll = new Collection("http://localhost:8080/coll", WBORecord);
    coll.get();
    do_check_eq(coll.iter.count, 1);

    let rec3 = coll.iter.next();
    do_check_eq(rec3.id, "record2");
    do_check_eq(rec3.modified, 2454725.98284);
    do_check_eq(typeof(rec3.payload), "object");
    do_check_eq(rec3.payload.cheese, "gruyere");

    log.info("Done!");
  }
  catch (e) { do_throw(e); }
  finally { server.stop(); }
}
