






if (typeof timeout == 'function' && typeof Worker != 'undefined') {
    
    
    JSTest.waitForExplicitFinish();
    expectExitCode(6);
    timeout(1.0);
    for (var i = 0; i < 5; i++)
        new Worker(workerDir + "worker-timeout-child.js"); 
} else {
    reportCompare(0, 0, "Test skipped. Shell workers and timeout required.");
}
