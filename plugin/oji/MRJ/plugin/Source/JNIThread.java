











































package netscape.oji;

public class JNIThread extends Thread {
	private int fSecureEnv;

	public JNIThread(int secureEnv) {
		super("JNIThread->0x" + Integer.toHexString(secureEnv));
		fSecureEnv = secureEnv;
		setPriority(NORM_PRIORITY);
		
		start();
	}
	
	public native void run();
}
