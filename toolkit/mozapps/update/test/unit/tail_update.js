





































if (gXHR) {
  gXHRCallback = null;
  
  gXHR.onerror = null;
  gXHR.onload = null;
  gXHR.onprogress = null;
  gXHR = null;
}

gUpdateChecker = null;
gAUS = null;
gPrefs = null;
gTestserver = null;
remove_dirs_and_files();
