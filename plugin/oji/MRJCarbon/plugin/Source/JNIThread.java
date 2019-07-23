












































package netscape.oji;

class JNIThread extends Thread {
	private int fSecureEnv;
	
	private JNIThread(int secureEnv) {
		super("JNIThread->0x" + Integer.toHexString(secureEnv));
		fSecureEnv = secureEnv;
		setPriority(NORM_PRIORITY);
		
		start();
	}
	
	public native void run();
}

class JNIRunnable implements Runnable {
    private int mJavaMessage;

	private JNIRunnable(int javaMessage) {
        mJavaMessage = javaMessage;
    }

    public native void run();
}
