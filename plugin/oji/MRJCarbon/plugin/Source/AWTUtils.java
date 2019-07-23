








































package netscape.oji;


import java.awt.Container;
import java.awt.Graphics;
import java.awt.Point;

import com.apple.mrj.internal.awt.PrintingPort;

public class AWTUtils {
	


	public static void printContainer(Container container, int printingPort, int originX, int originY, Object notifier) {
		try {
			
			PrintingPort printer = new PrintingPort(printingPort, originX, originY);
			Graphics graphics = printer.getGraphics(container);

			
			container.printAll(graphics);
			
			graphics.dispose();
			printer.dispose();
		} finally {
			
			if (notifier != null) {
				synchronized(notifier) {
					notifier.notifyAll();
				}
			}
		}
	}
}
