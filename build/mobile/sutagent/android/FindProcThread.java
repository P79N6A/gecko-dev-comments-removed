




































package com.mozilla.SUTAgentAndroid.service;

import java.util.List;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.ContextWrapper;

public class FindProcThread extends Thread {
    ContextWrapper    contextWrapper = null;
    String sProcNameToFind = "";
    boolean bFoundIt = false;
    boolean bStillRunning = true;

    public FindProcThread(ContextWrapper ctx, String sProcessName) {
        this.contextWrapper = ctx;
        this.sProcNameToFind = sProcessName;
        this.bFoundIt = false;
    }

    public void run() {
        ActivityManager aMgr = (ActivityManager) contextWrapper.getSystemService(Activity.ACTIVITY_SERVICE);
        List <ActivityManager.RunningAppProcessInfo> lProcesses;
        int lcv = 0;
        int nNumLoops = 0;
        String strProcName = "";
        int    nPID = 0;

        if (aMgr == null)
            return;


        
        while (bStillRunning && !bFoundIt) {
            lProcesses = aMgr.getRunningAppProcesses();
            if (lProcesses != null) {
                for (lcv = 0; lcv < lProcesses.size(); lcv++) {
                    if (lProcesses.get(lcv).processName.contains(sProcNameToFind)) {
                        strProcName = lProcesses.get(lcv).processName;
                        nPID = lProcesses.get(lcv).pid;
                        bFoundIt = true;
                        break;
                    }
                }
                if (bFoundIt)             
                    continue;
            }
            try {
                Thread.sleep(500);         
                if (++nNumLoops > 10) { 
                    bStillRunning = false;
                }
                lProcesses = null;
                System.gc();
                Thread.yield();
            } catch (InterruptedException e) {
                e.printStackTrace();
                bStillRunning = false;
            }
        }
    }

}
