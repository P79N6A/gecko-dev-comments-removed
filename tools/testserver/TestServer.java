




































import java.io.*;
import java.net.*;

public class TestServer extends Thread {

	final static int DEFAULT_PORT = 4321;

    protected int port;
	protected ServerSocket listen_socket;

	public static void fail(Exception e, String msg) {
		System.err.println(msg + ":" + e);
		e.printStackTrace();
	}

	public TestServer() {
		this(DEFAULT_PORT);
	}

	public TestServer(int port) {
		this.port = port;
		try {
			listen_socket = new ServerSocket(port);
		}
		catch (IOException e) {
			fail(e, "Exception creating server socket");
		}
		System.out.println("Server: listening on port " + port);
		this.start();
	}

	public void run() {
		try {
			while (true) {
				Socket client_socket = listen_socket.accept();
				new Connection(client_socket);
			}
		}
		catch (IOException e) {
			fail(e, "Exception while listening");
		}
	}

	public static void main(String args[]) {
		int port = DEFAULT_PORT;
		if (args.length == 1) {
			try {
				port = Integer.parseInt(args[0]);
			}
			catch (NumberFormatException e) {
				port = DEFAULT_PORT;
			}
		}
		new TestServer(port);
	}

}

