


function run_test()
{
  
  

  add_test(test_error);

  run_next_test();
}
function test_error()
{
  
  do_test_pending();
  received = false;
  id = "";

  let transport = debuggerSocketConnect("127.0.0.1", 2828);
  transport.hooks = {
    onPacket: function(aPacket) {
      this.onPacket = function(aPacket) {
        if (received) {
          do_check_eq(aPacket.from, id);
          if(aPacket.error == undefined) {
            do_throw("Expected error, instead received 'done' packet!");
            transport.close();
          }
          else {
            transport.close();
          }
        }
        else {
          received = true;
          id = aPacket.id;
          transport.send({to: id,
                        type: "nonExistent",
                        });
        }
      }
      transport.send({to: "root",
                      type: "getMarionetteID",
                      });
    },
    onClosed: function(aStatus) {
      do_check_eq(aStatus, 0);
      do_test_finished();
      run_next_test();
    },
  };
  transport.ready();
}
