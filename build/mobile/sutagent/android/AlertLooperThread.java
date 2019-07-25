




































package com.mozilla.SUTAgentAndroid;

import java.util.Timer;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;

class AlertLooperThread extends Thread
	{
	public Handler mHandler;
	private Looper looper = null;
	private DoAlert da	= null;
	private Timer alertTimer = null;
	
	public Timer getAlertTimer()
		{
		return alertTimer;
		}

	public void term()
		{
		if (da != null)
			da.term();
		}

	public void quit()
		{
		if (looper != null)
			looper.quit();
		}

	public void run()
		{
		Looper.prepare();
    
		looper = Looper.myLooper();
      
		mHandler = new Handler()
    		{
			public void handleMessage(Message msg)
        		{
				
        		}
    		};
      
    	alertTimer = new Timer();
    	da = new DoAlert();
    	alertTimer.scheduleAtFixedRate(da, 0, 5000);
    	Looper.loop();
		}
	}
