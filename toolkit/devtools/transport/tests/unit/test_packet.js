


const { JSONPacket, BulkPacket } =
  devtools.require("devtools/toolkit/transport/packets");

function run_test() {
  add_test(test_packet_done);
  run_next_test();
}


function test_packet_done() {
  let json = new JSONPacket();
  do_check_false(!!json.done);

  let bulk = new BulkPacket();
  do_check_false(!!bulk.done);

  run_next_test();
}
