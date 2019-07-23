




































import java.io.*;
import java.util.Date;
import java.util.StringTokenizer;





class ScriptFile {

    public static final String URLMAP = new String("docs/urlmap");

	public ScriptFile(Connection c, String req, PrintWriter stream ) {
        con = c;
	    request = req;
	    out = stream;
	    if (out == null)
	        out = new PrintWriter(System.out);
	    file = URLMAP;
	    try {
	        Parse();
	    }
	    catch (IOException e)
	    {
	        out.println("HTTP/1.1 500 Server Error\n\n");
	        out.println("Failed with this exception-");
	        out.println(e);
	    }
	    out.flush();
	}

	public ScriptFile(String f) {
		file = f;
		out = new PrintWriter(System.out);
	    try {
	        Parse();
	    }
	    catch (FileNotFoundException e)
	    {
	        out.println("HTTP/1.1 404 File Not Found\n\n");
	        out.println("File not found!");
	        out.println(e);
	    }
	    catch (IOException e)
	    {
	        out.println("HTTP/1.1 500 Server Error\n\n");
	        out.println("Failed with this exception-");
	        out.println(e);
	    }
	    out.flush();
	}

	public void Parse () throws IOException {

	    if ((request == null) ||
			(request.length() == 0) ||
			request.startsWith("GET / ") || request.startsWith("get / "))
	    {
	        WriteDefaultResponse();
            return;
	    }

		if (request.startsWith("GET /?") || request.startsWith("get /?"))
		{
			WriteSpecialResponse(request.substring(request.indexOf('?')+1));
			return;
		}

        if (request.startsWith("HEAD /") || request.startsWith("head /"))
        {
            WriteHeadResponse();
            return;
        }

        boolean outDirty = false;
	    if (file != null)
	    {
			  BufferedReader in = new BufferedReader(
			  	new InputStreamReader(
					new FileInputStream(file)));

              String s = new String();
              while((s = in.readLine())!= null)
              {
                s.trim();
                
                if ((s.length() == 0) || (s.charAt(0) == '#'))
                    continue;
                if (s.startsWith("START")) {
                    
                    String filename = new String ("GET " + s.substring(6));
                    String otherfilename = new String("POST " + s.substring(6));
                    if (request.startsWith(filename) || 
                            request.startsWith(otherfilename))
                    {
                        continue;
                    }
                    else
                    {
                        while (!s.startsWith("END"))
                        {
                            s = in.readLine();
                            if (s != null)
                            {
                                s.trim();
                            }
                            else
                                break;
                        }
                    }
                }
                else if (s.startsWith("ECHO")) {
                    outDirty = true;
                    boolean parameter = false;
                    try {
                        String header = new String(s.substring(5));
                        String req= new String(con.request);
                        int t = req.indexOf(header);
                        if (t != -1) {
                            out.println(req.substring(
                                t, req.indexOf("\n", t)));
                            parameter = true;
                        }
                        else {
                            out.println("Error: " + header + 
                                    " not specified in request!");
                        }
                        
                    }
                    catch (StringIndexOutOfBoundsException e) {}
                    if (!parameter)
                        out.println(con.request);
                }
                else if (s.startsWith("INCLUDE")) {
                    outDirty = true;
                    WriteOutFile("docs/" + s.substring(8));
                }
                else if (s.startsWith("CRLF")) {
                    outDirty = true;
                    out.println();
                }
                else if (s.startsWith("END")) {
                    
                    continue;
                }
                else {
                    outDirty = true;
                    out.println(s);
                }

              }
              in.close();

              if (outDirty)
              {
                out.flush();
              }
              else
                WriteDefaultResponse();
	    }
	}

	public static void main(String args[]) {
	    if (args.length >= 1) {
            ScriptFile sf = new ScriptFile(args[0]);
            






	    }
	}

    protected void WriteOutFile(String filename) throws IOException {
		  BufferedReader incl = new BufferedReader(
			new InputStreamReader(
               new FileInputStream(filename)));
        
        String s;
        while ((s = incl.readLine()) != null)
        {
            out.println(s);
        }
        incl.close();
    }

    protected void WriteDefaultResponse() throws IOException {
		WriteOutFile("docs/generic.res");
        out.println("Date: " + (new Date()).toString());
        if (file != null)
        {
            File f = new File(file);
            out.println("Last-modified: " + (new Date(f.lastModified())).toString());
        }
        out.println("\n"); 
        if (con != null)
        {
			out.println("<HTML><BODY>");
            out.println("<H3>Rec'd. the following request-</H3>");
            out.println("<PRE>");
            out.println(con.request);
            out.println("</PRE>");
            out.println("From- " + con.client);
			out.println("</BODY></HTML>");
         }
    }

	protected void WriteSpecialResponse(String specialcase) throws IOException {
        out.println("HTTP/1.1 200 OK");
        out.println("Server: HTTP Test Server/1.1");
        out.println("Content-Type: text/plain");

		StringTokenizer st = new StringTokenizer(specialcase, " &");
		while (st.hasMoreTokens()) {
		    String pair = st.nextToken();
    		if (pair.startsWith("Length=") || pair.startsWith("length="))
    		{

                int len = Integer.valueOf(pair.substring(
                            pair.indexOf('=')+1), 10).intValue();

    			out.println("Date: " + (new Date()).toString());
    			out.println("Content-Length: " + len);
    			out.println("\n");
    			for (int i = 0; i<len; i++) {
                    out.print ((char)i%10);
                    if ((i>0) && ((i+1)%80) == 0)
    			        out.print('\n');
    			    out.flush();
    			}
    			return;
    		}
    	}

	}

    protected void WriteHeadResponse() {
        out.println("HTTP/1.1 200 OK");
        out.println("Server: HTTP Test Server/1.1");
        out.println("Content-Type: text/plain");
        out.println("Content-Length: 1000" ); 
        out.println("Date: " + (new Date()).toString());
        out.println(); 
        out.flush();
    }

    String file = null;
    

    String request;
    PrintWriter out;
    Connection con;
}
