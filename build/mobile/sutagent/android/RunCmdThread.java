




































package com.mozilla.SUTAgentAndroid;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.ArrayList;
import java.util.List;

import android.widget.Toast;

public class RunCmdThread extends Thread
	{
	private ServerSocket SvrSocket = null;
	private Socket socket	= null;
	boolean bListening	= true;
	List<CmdWorkerThread> theWorkers = new ArrayList<CmdWorkerThread>();
	
	public RunCmdThread(ServerSocket socket)
		{
		super("RunCmdThread");
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
					CmdWorkerThread theWorker = new CmdWorkerThread(this, socket);
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
			
			SUTAgentAndroid.me.finish();
			} 
		catch (IOException e)
			{
			Toast.makeText(SUTAgentAndroid.me.getApplicationContext(), e.getMessage(), Toast.LENGTH_LONG).show();
		    e.printStackTrace();
			}
		return;
	}
}
