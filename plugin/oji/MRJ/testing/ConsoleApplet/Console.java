







package netscape.console;

import java.io.*;
import java.awt.*;

public class Console {
	
	private static InputStream in;
	private static PrintStream out;
	private static PrintStream err;
	private static TextArea console;

	private static class Input extends InputStream {
		byte[] buffer = new byte[1024];
		int position = 0;
		int count = 0;
		
		private void fillBuffer() throws EOFException {
			
			int length = 1024;
			if (length == -1)
				throw new EOFException();
			count = length;
			position = 0;
		}
		
	    public int read() throws IOException {
	    	synchronized(this) {
		    	if (position >= count)
		    		fillBuffer();
    			return buffer[position++];
    		}
	    }

	    public int read(byte[] b, int offset, int length) throws IOException {
			synchronized(this) {
				
		    	if (position >= count)
		    		fillBuffer();
		    	int initialOffset = offset;
		    	while (offset < length && position < count) {
		    		b[offset++] = buffer[position++];
		    	}
		    	return (offset - initialOffset);
		    }
	    }
	}
	
	private static class Output extends OutputStream implements Runnable {
		StringBuffer buffer = new StringBuffer();
		
		public Output() {
			Thread flusher = new Thread(this, getClass().getName() + "-Flusher");
			flusher.setDaemon(true);
			flusher.start();
		}
	
		public synchronized void write(int b) throws IOException {
			this.buffer.append((char)b);
			notify();
	    }

		public synchronized void write(byte[] buffer, int offset, int count) throws IOException {
			this.buffer.append(new String(buffer, 0, offset, count));
			notify();
		}

	    public synchronized void flush() throws IOException {
	    	String value = this.buffer.toString();
	    	console.append(value);
	    	this.buffer.setLength(0);
    	}
		
    	



    	public synchronized void run() {
   			for (;;) {
  		  		try {
    				wait();
    				flush();
	    		} catch (InterruptedException ie) {
	    		} catch (IOException ioe) {
    			}
    		}
    	}
	}
	
	private static class Error extends Output {}

	public static void init(TextArea text) {
		in = System.in;
		out = System.out;
		err = System.err;
		console = text;

		System.setIn(new Input());
		System.setOut(new PrintStream(new Output()));
		System.setErr(new PrintStream(new Error()));
	}
	
	public static void dispose() {
		System.setIn(in);
		System.setOut(out);
		System.setErr(err);
		
		console = null;
	}
}
