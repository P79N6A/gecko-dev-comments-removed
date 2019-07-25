




































package com.mozilla.SUTAgentAndroid;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.text.SimpleDateFormat;
import java.util.Calendar;

public class DataWorkerThread extends Thread
{
	private RunDataThread theParent = null;
	private Socket socket	= null;
	boolean bListening	= true;

	public DataWorkerThread(RunDataThread theParent, Socket workerSocket)
		{
		super("DataWorkerThread");
		this.theParent = theParent;
		this.socket = workerSocket;
		}

	public void StopListening()
		{
		bListening = false;
		}

	private String readLine(BufferedInputStream in)
		{
		String sRet = "";
		int nByte = 0;
		char cChar = 0;
	
		try 
			{
			nByte = in.read();
			while (nByte != -1)
				{
				cChar = ((char)(nByte & 0xFF));
				if ((cChar != '\r') && (cChar != '\n'))
					sRet += cChar;
				else
					break;
				nByte = in.read();
				}
		
			if (in.available() > 0)
				{
				in.mark(1024);
				nByte = in.read();
		
				while (nByte != -1)
					{
					cChar = ((char)(nByte & 0xFF));
					if ((cChar == '\r') || (cChar == '\n'))
						{
						if (in.available() > 0)
							{
							in.mark(1024);
							nByte = in.read();
							}
						else
							nByte = -1;
						}
					else
						{
						in.reset();
						break;
						}
					}
				}
			}
		catch (IOException e)
			{
			
			e.printStackTrace();
			}
	
		if (sRet.length() == 0)
			sRet = null;
	
		return(sRet);
		}

	public void run()
		{
		String	sRet = "";
		
		try {
			while(bListening)
				{
				OutputStream cmdOut = socket.getOutputStream();
				InputStream cmdIn = socket.getInputStream();
				PrintWriter out = new PrintWriter(cmdOut, true);
				BufferedInputStream in = new BufferedInputStream(cmdIn);
				String inputLine, outputLine;
				DoCommand dc = new DoCommand();
				
	    		Calendar cal = Calendar.getInstance();
	    		SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMdd-HH:mm:ss");
	    		sRet = sdf.format(cal.getTime());
	    		sRet += " trace output";

				out.println(sRet);
				out.flush();
				int nAvail = cmdIn.available();
				cmdIn.skip(nAvail);
				
				((SUTAgentAndroid)SUTAgentAndroid.me).StartHeartBeat(out);

				while (bListening)
					{
					if (!(in.available() > 0))
						{
						socket.setSoTimeout(500);
						try {
							int nRead = cmdIn.read();
							if (nRead == -1)
								{
								bListening = false;
								continue;
								}
							else
								inputLine = (char)nRead + ""; 
							}
						catch(SocketTimeoutException toe)
							{
							continue;
							}
						}
					else
						inputLine = "";
				
					if ((inputLine += readLine(in)) != null)
						{
						outputLine = dc.processCommand(inputLine, out, in, cmdOut);
						out.print(outputLine + "\n");
						out.flush();
						if (outputLine.equals("exit"))
							{
							theParent.StopListening();
							bListening = false;
							}
						}
					else
						break;
					}
				
				((SUTAgentAndroid)SUTAgentAndroid.me).StopHeartBeat();

				out.close();
				in.close();
				socket.close();
				}
			}
		catch (IOException e)
			{
			
			e.printStackTrace();
			}
		}
}
