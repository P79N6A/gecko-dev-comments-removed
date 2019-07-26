if (gClient) {
  
  gClient.close(function () {
    run_next_test();
  });
}
