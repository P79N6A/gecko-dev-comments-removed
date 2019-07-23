





































var rv;

rv = initInstall("JSSh JavaScript Shell Server", "JSSh", "0.1");
logComment("initInstall: " + rv);

var programFolder = getFolder("Program");
logComment("programFolder: "+programFolder);

rv = addDirectory("Program", "0.1", "bin", programFolder, "", true);
logComment("addDirectory(bin): " + rv);

var chromeFolder = getFolder("Chrome", "jssh.jar");
logComment("chromeFolder: "+chromeFolder);

rv = registerChrome(CONTENT | DELAYED_CHROME, chromeFolder, "content/jssh/");

if (rv == ACCESS_DENIED) {
  alert("Unable to write to program directory " + programFolder + ".\n You will need to restart the browser with administrator/root privileges to install this software. After installing as root (or administrator), you will need to restart the browser one more time to register the installed software.\n After the second restart, you can go back to running the browser without privileges!");
  cancelInstall(rv);
  logComment("cancelInstall() due to error: " + rv);
}
else if (rv != SUCCESS) {
  cancelInstall(rv);
  logComment("cancelInstall() due to error: " + rv);
}
else {
  performInstall(); 
  logComment("performInstall(): " + rv);
}
