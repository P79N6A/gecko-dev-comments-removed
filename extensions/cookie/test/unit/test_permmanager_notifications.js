





let test_generator = do_run_test();

function run_test() {
  do_test_pending();
  test_generator.next();
}

function continue_test()
{
  do_run_generator(test_generator);
}

function do_run_test() {
  
  let profile = do_get_profile();

  let pm = Services.permissions;
  let permURI = NetUtil.newURI("http://example.com");
  let now = (new Date()).getTime();
  let permType = "test/expiration-perm";

  let observer = new permission_observer(test_generator, now, permType);
  Services.obs.addObserver(observer, "perm-changed", false);

  
  
  
  do_execute_soon(function() {
    pm.add(permURI, permType, pm.ALLOW_ACTION, pm.EXPIRE_TIME, now + 100000);
  });
  yield;

  
  do_execute_soon(function() {
    pm.add(permURI, permType, pm.ALLOW_ACTION, pm.EXPIRE_TIME, now + 200000);
  });
  yield;

  
  do_execute_soon(function() {
    pm.remove(permURI.asciiHost, permType);
  });
  yield;

  
  do_execute_soon(function() {
    pm.removeAll();
  });
  yield;

  Services.obs.removeObserver(observer, "perm-changed");
  do_check_eq(observer.adds, 1);
  do_check_eq(observer.changes, 1);
  do_check_eq(observer.deletes, 1);
  do_check_true(observer.cleared);

  do_finish_generator_test(test_generator);
}

function permission_observer(generator, now, type) {
  
  this.generator = generator;
  this.pm = Services.permissions;
  this.now = now;
  this.type = type;
  this.adds = 0;
  this.changes = 0;
  this.deletes = 0;
  this.cleared = false;
}

permission_observer.prototype = {
  observe: function(subject, topic, data) {
    do_check_eq(topic, "perm-changed");

    
    
    
    
    if (data == "added") {
      var perm = subject.QueryInterface(Ci.nsIPermission);
      this.adds++;
      switch (this.adds) {
        case 1:
          do_check_eq(this.type, perm.type);
          do_check_eq(this.pm.EXPIRE_TIME, perm.expireType);
          do_check_eq(this.now + 100000, perm.expireTime);
          break;
        default:
          do_throw("too many add notifications posted.");
      }

    } else if (data == "changed") {
      let perm = subject.QueryInterface(Ci.nsIPermission);
      this.changes++;
      switch (this.changes) {
        case 1:
          do_check_eq(this.type, perm.type);
          do_check_eq(this.pm.EXPIRE_TIME, perm.expireType);
          do_check_eq(this.now + 200000, perm.expireTime);
          break;
        default:
          do_throw("too many change notifications posted.");
      }

    } else if (data == "deleted") {
      var perm = subject.QueryInterface(Ci.nsIPermission);
      this.deletes++;
      switch (this.deletes) {
        case 1:
          do_check_eq(this.type, perm.type);
          break;
        default:
          do_throw("too many delete notifications posted.");
      }

    } else if (data == "cleared") {
      
      do_check_false(this.cleared);
      do_check_eq(do_count_enumerator(Services.permissions.enumerator), 0);
      this.cleared = true;

    } else {
      do_throw("unexpected data '" + data + "'!");
    }

    
    do_run_generator(this.generator);
  },
};

