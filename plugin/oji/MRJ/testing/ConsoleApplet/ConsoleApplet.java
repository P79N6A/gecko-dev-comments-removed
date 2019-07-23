















































package netscape.console;

import java.io.*;
import java.awt.*;
import java.applet.*;
import java.awt.event.*;

public class ConsoleApplet extends Applet {
	TextArea console;

	public ConsoleApplet() {
		setLayout(new BorderLayout());	
		add(console = new TextArea(), BorderLayout.CENTER);

		Panel panel = new Panel();
		add(panel, BorderLayout.SOUTH);

		
		ActionListener clearConsoleListener = new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				console.setText("");
			}
		};

		Button clearConsole = new Button("Clear");
		clearConsole.addActionListener(clearConsoleListener);
		panel.add(clearConsole);

		
		ActionListener dumpThreadsListener = new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				dumpThreads();
			}
		};

		Button dumpThreads = new Button("Dump Threads");
		dumpThreads.addActionListener(dumpThreadsListener);
		panel.add(dumpThreads);
	}
	
	public void init() {
		Console.init(console);
	}

	public void destroy() {
		Console.dispose();
	}
	
	public void dumpThreads() {
		System.out.println("Dumping threads...");
	}
}
