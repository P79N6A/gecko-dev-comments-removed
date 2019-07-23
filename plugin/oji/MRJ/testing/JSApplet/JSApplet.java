











































import java.io.*;
import java.awt.*;
import java.applet.*;
import java.awt.event.*;

import netscape.javascript.JSObject;

public class JSApplet extends Applet {
	TextField text;

	public void init() {
		setLayout(new BorderLayout());	
		add(text = new TextField(), BorderLayout.CENTER);

		Panel panel = new Panel();
		add(panel, BorderLayout.SOUTH);

		
		ActionListener evalListener = new ActionListener() {
			JSObject window;
		
			public void actionPerformed(ActionEvent e) {
				if (window == null)
					window = JSObject.getWindow(JSApplet.this);
				Object result = window.eval(text.getText());
				if (result != null)
					System.out.println(result);
				text.selectAll();
			}
		};

		Button evalButton = new Button("eval");
		evalButton.addActionListener(evalListener);
		text.addActionListener(evalListener);
		panel.add(evalButton);

		
		ActionListener clearConsoleListener = new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				text.setText("");
			}
		};

		Button clearConsole = new Button("clear");
		clearConsole.addActionListener(clearConsoleListener);
		panel.add(clearConsole);
	}

	public void destroy() {
	}
}
