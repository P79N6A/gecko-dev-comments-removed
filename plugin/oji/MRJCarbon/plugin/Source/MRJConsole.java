

















































package netscape.oji;

import java.io.*;
import java.awt.*;
import java.awt.event.*;

class ConsoleWindow extends Frame {
	TextArea text;

	ConsoleWindow(String title) {
		super("Java Console");
		
		addWindowListener(
			new WindowAdapter() {
				public void windowClosing(WindowEvent e) {
					hide();
				}
			});
		
		add(text = new TextArea());
		setSize(300, 200);

		ActionListener dumpThreadsListener =
			new ActionListener() {
				public void actionPerformed(ActionEvent e) {
					dumpThreads();
				}
			};

		
		MenuBar menuBar = new MenuBar();
		Menu consoleMenu = new Menu("Console");
		consoleMenu.add(newItem("Dump Threads", dumpThreadsListener));
		
		menuBar.add(consoleMenu);
		setMenuBar(menuBar);
	}

	private MenuItem newItem(String title, ActionListener listener) {
		MenuItem item = new MenuItem(title);
		item.addActionListener(listener);
		return item;
	}
	
	public void dumpThreads() {
		System.out.println("Dumping threads...");
	}
}

public class MRJConsole {
	
	private static InputStream in;
	private static PrintStream out;
	private static PrintStream err;
	private static ConsoleWindow window;

	native static int readLine(byte[] buffer, int offset, int length);
	native static void writeLine(byte[] buffer, int offset, int length);

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
	    	window.text.append(value);
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

	public static void init() {
		in = System.in;
		out = System.out;
		err = System.err;

		window = new ConsoleWindow("Java Console");

		System.setIn(new Input());
		System.setOut(new PrintStream(new Output()));
		System.setErr(new PrintStream(new Error()));

		done();
	}
	
	public static void dispose() {
		System.setIn(in);
		System.setOut(out);
		System.setErr(err);
		window.dispose();
		window = null;
		done();
	}
	
	public static void show() {
		window.show();
		done();
	}
	
	public static void hide() {
		window.hide();
		done();
	}
	
	public static void visible(boolean[] result) {
		result[0] = window.isVisible();
		done();
	}
	
	public static void print(String text) {
		System.out.print(text);
		done();
	}
	
	public static synchronized void finish() {
		try {
			MRJConsole.class.wait();
		} catch (InterruptedException ie) {
		}
	}
	
	private static synchronized void done() {
		MRJConsole.class.notify();
	}
}
