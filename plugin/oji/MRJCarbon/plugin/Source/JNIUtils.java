








































package netscape.oji;

public class JNIUtils {
	


	public static Object NewLocalRef(Object object) {
		return object;
	}
	
	


	public static Object GetCurrentThread() {
		return Thread.currentThread();
	}
	
	
	


	static class StubSecurityManager extends SecurityManager {
		public ClassLoader getCurrentClassLoader() {
			return currentClassLoader();
		}
	}
	
	private static StubSecurityManager stubManager = new StubSecurityManager();
	
	


	public static Object GetCurrentClassLoader() {
		return stubManager.getCurrentClassLoader();
	}
	
	public static Object GetObjectClassLoader(Object object) {
		return object.getClass().getClassLoader();
	}
}
