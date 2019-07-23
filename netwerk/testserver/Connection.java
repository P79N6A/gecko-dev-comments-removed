




































import java.io.*;
import java.net.*;

class Connection extends Thread {

    public static final boolean DEBUG = true;
	public Socket client;
	protected BufferedReader in;
	protected PrintWriter out;

    final static String SERVER = "HTTP Test Server/1.1";

    public Connection(Socket client_socket) {
		client = client_socket;

		if (DEBUG)
		{
			System.out.println(
			    "---------------------------------------------------------");
			System.out.println(client);
		}
		try {
			in = new BufferedReader(new InputStreamReader(
				client.getInputStream()));
			out = new PrintWriter(client.getOutputStream());
			this.start();
		}
		catch (IOException e) {
			try {
				client.close();
			}
			catch (IOException another) {
			}
			System.out.println("Exception while getting socket streams."
				+ e);
		}
	}

	final public void run() {

		String line = null;
		String firstline = null;
		request = new StringBuffer();

		int len = 0;
        int extralen= 0;
		try {
			while(true) {
				if (!in.ready()) {
					continue;
			    }
                line = in.readLine();
				if (line == null)
					break;
				if (firstline == null)
				    firstline = new String(line);
				len = line.length();
                if (line.regionMatches(true, 0,
                        "Content-length: ", 0, 16)) {
                    extralen = Integer.valueOf(line.substring(16,
                        line.length())).intValue();
                }
				if (len == 0)
                {
                    
                    if (extralen > 0)
                    {
                        char[] postbuffer = new char[extralen];
                        in.read(postbuffer);
                        request.append("\n");
                        request.append(postbuffer);
                        if (DEBUG) {
                            System.out.println();
                            System.out.println(postbuffer);
                        }
                    }
                    break;
                }
                request.append(line);
				request.append("\n");
				if (DEBUG)
				    System.out.println(line);
			}
            
            
            ScriptFile sf = new ScriptFile(this, firstline, out);
    		out.flush();
    		
    		sf = null;
		} 
		catch (IOException e) {
			System.out.println("Some problem while communicating. " + e);
		}
		finally {
			try {
				client.close();
			}
			catch (IOException e2) {
			}
		}
	}
	StringBuffer request;
}
