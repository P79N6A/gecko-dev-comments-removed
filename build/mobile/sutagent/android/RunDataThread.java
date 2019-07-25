




































package com.mozilla.SUTAgentAndroid;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.ArrayList;
import java.util.List;
import java.util.Timer;

import android.widget.Toast;

public class RunDataThread extends Thread
	{
	Timer heartBeatTimer;
	
	private ServerSocket SvrSocket = null;
	private Socket socket	= null;
	boolean bListening	= true;
	List<DataWorkerThread> theWorkers = new ArrayList<DataWorkerThread>();
	
	public RunDataThread(ServerSocket socket)
		{
		super("RunDataThread");
		this.SvrSocket = socket;
		}
	
	public void StopListening()
		{
		bListening = false;
		}

	public void run() {
		try {
			SvrSocket.setSoTimeout(5000);
			while (bListening)
				{
				try 
					{
					socket = SvrSocket.accept();
					DataWorkerThread theWorker = new DataWorkerThread(this, socket);
					theWorker.start();
					theWorkers.add(theWorker);
					}
				catch (SocketTimeoutException toe)
					{
					continue;
					}
				}
			
			int nNumWorkers = theWorkers.size();
			for (int lcv = 0; lcv < nNumWorkers; lcv++)
				{
				if (theWorkers.get(lcv).isAlive())
					{
					theWorkers.get(lcv).StopListening();
					while(theWorkers.get(lcv).isAlive())
						;
					}
				}
			
			theWorkers.clear();
			
			SvrSocket.close();
			}
		catch (IOException e)
			{
			Toast.makeText(SUTAgentAndroid.me.getApplicationContext(), e.getMessage(), Toast.LENGTH_LONG).show();
			e.printStackTrace();
			}
		return;
		}
	}
