




#ifndef MacLaunchHelper_h_
#define MacLaunchHelper_h_

extern "C" {
  void LaunchChildMac(int aArgc, char** aArgv, PRUint32 aRestartType = 0,
                      pid_t *pid = 0);
}

#endif
