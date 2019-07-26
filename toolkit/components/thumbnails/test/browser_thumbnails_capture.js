






function runTests() {
  
  yield addTab("data:text/html,<body bgcolor=ff0000></body>");
  yield captureAndCheckColor(255, 0, 0, "we have a red thumbnail");

  
  yield navigateTo("data:text/html,<body bgcolor=00ff00></body>");
  yield captureAndCheckColor(0, 255, 0, "we have a green thumbnail");

  
  yield navigateTo("data:text/html,<body bgcolor=0000ff></body>");
  yield captureAndCheckColor(0, 0, 255, "we have a blue thumbnail");
}
