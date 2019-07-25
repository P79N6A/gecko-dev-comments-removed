




































package com.mozilla.SUTAgentAndroid.service;

import java.net.ServerSocket;
import java.util.Timer;

import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;

import android.os.Handler;
import android.os.IBinder;
import android.widget.Toast;

public class ASMozStub extends android.app.Service {
	
	private ServerSocket cmdChnl = null;
	private ServerSocket dataChnl = null;
	private Handler handler = new Handler();
	RunCmdThread runCmdThrd = null;
	RunDataThread runDataThrd = null;
	Thread monitor = null;
	Timer timer = null;
	
	@Override
	public IBinder onBind(Intent intent)
		{
		return null;
		}
	
	@Override
	public void onCreate() {
		super.onCreate();
		Toast.makeText(this, "Listener Service created...", Toast.LENGTH_LONG).show();
		}

	public void onStart(Intent intent, int startId) {
		super.onStart(intent, startId);
		
		try {
			cmdChnl = new ServerSocket(20701);
			runCmdThrd = new RunCmdThread(cmdChnl, this, handler);
			runCmdThrd.start();
			Toast.makeText(this, "Command channel port 20701 ...", Toast.LENGTH_LONG).show();
			
			dataChnl = new ServerSocket(20700);
			runDataThrd = new RunDataThread(dataChnl, this);
			runDataThrd.start();
			Toast.makeText(this, "Data channel port 20700 ...", Toast.LENGTH_LONG).show();
			} 
		catch (Exception e) {

			}
		
		return;
		}
	
	public void onDestroy()
		{
		super.onDestroy();
		if (runCmdThrd.isAlive())
			{
			runCmdThrd.StopListening();
			}
		
		if (runDataThrd.isAlive())
			{
			runDataThrd.StopListening();
			}
		
		NotificationManager notificationManager = (NotificationManager)getSystemService(Context.NOTIFICATION_SERVICE);
		notificationManager.cancel(1959);
		
		Toast.makeText(this, "Listener Service destroyed...", Toast.LENGTH_LONG).show();
		
		System.exit(0);
		}
	
	public void SendToDataChannel(String strToSend)
		{
		if (runDataThrd.isAlive())
			{
			runDataThrd.SendToDataChannel(strToSend);
			}
		}
}
