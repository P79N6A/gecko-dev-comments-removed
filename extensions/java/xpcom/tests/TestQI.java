



































import java.io.File;
import java.io.FileFilter;
import java.io.IOException;

import org.mozilla.xpcom.Mozilla;
import org.mozilla.interfaces.nsIFile;
import org.mozilla.interfaces.nsIFileURL;
import org.mozilla.interfaces.nsIServiceManager;
import org.mozilla.interfaces.nsISupports;
import org.mozilla.interfaces.nsIURI;
import org.mozilla.interfaces.nsIURL;










public class TestQI {

	private static File grePath;

	


	public static void main(String[] args) {
		try {
			checkArgs(args);
		} catch (IllegalArgumentException e) {
			System.exit(-1);
		}

		Mozilla mozilla = Mozilla.getInstance();
		mozilla.initialize(grePath);

		File profile = null;
		nsIServiceManager servMgr = null;
		try {
			profile = createTempProfileDir();
			LocationProvider locProvider = new LocationProvider(grePath,
					profile);
			servMgr = mozilla.initXPCOM(grePath, locProvider);
		} catch (IOException e) {
			e.printStackTrace();
			System.exit(-1);
		}

		try {
			runTest();
		} catch (Exception e) {
			e.printStackTrace();
			System.exit(-1);
		}

		System.out.println("Test Passed.");

		
		mozilla.shutdownXPCOM(servMgr);
		deleteDir(profile);
	}

	private static void runTest() {
		FooFile foo = new FooFile();

		nsIFileURL fileURL = (nsIFileURL) foo
				.queryInterface(nsIFileURL.NS_IFILEURL_IID);
		if (fileURL == null) {
			throw new RuntimeException("Failed to QI to nsIFileURL.");
		}

		nsIURL url = (nsIURL) foo.queryInterface(nsIURL.NS_IURL_IID);
		if (url == null) {
			throw new RuntimeException("Failed to QI to nsIURL.");
		}

		nsIURI uri = (nsIURI) foo.queryInterface(nsIURI.NS_IURI_IID);
		if (uri == null) {
			throw new RuntimeException("Failed to QI to nsIURI.");
		}

		nsISupports supp = (nsISupports) foo
				.queryInterface(nsISupports.NS_ISUPPORTS_IID);
		if (supp == null) {
			throw new RuntimeException("Failed to QI to nsISupports.");
		}
	}

	private static void checkArgs(String[] args) {
		if (args.length != 1) {
			printUsage();
			throw new IllegalArgumentException();
		}

		grePath = new File(args[0]);
		if (!grePath.exists() || !grePath.isDirectory()) {
			System.err.println("ERROR: given path doesn't exist");
			printUsage();
			throw new IllegalArgumentException();
		}
	}

	private static void printUsage() {
		
	}

	private static File createTempProfileDir() throws IOException {
		
		File profile = File.createTempFile("mozilla-test-", null);
		profile.delete();

		
		
		File[] files = profile.getParentFile()
				.listFiles(new FileFilter() {
					public boolean accept(File file) {
						if (file.getName().startsWith("mozilla-test-")) {
							return true;
						}
						return false;
					}
				});
		for (int i = 0; i < files.length; i++) {
			deleteDir(files[i]);
		}

		
		profile.mkdir();

		return profile;
	}

	private static void deleteDir(File dir) {
		File[] files = dir.listFiles();
		for (int i = 0; i < files.length; i++) {
			if (files[i].isDirectory()) {
				deleteDir(files[i]);
			}
			files[i].delete();
		}
		dir.delete();
	}	

}














class FooFile implements nsIFileURL {

	public nsISupports queryInterface(String aIID) {
		return Mozilla.queryInterface(this, aIID);
	}

	public nsIFile getFile() {
		return null;
	}

	public void setFile(nsIFile aFile) {
	}

	public String getFilePath() {
		return null;
	}

	public void setFilePath(String aFilePath) {
	}

	public String getParam() {
		return null;
	}

	public void setParam(String aParam) {
	}

	public String getQuery() {
		return null;
	}

	public void setQuery(String aQuery) {
	}

	public String getRef() {
		return null;
	}

	public void setRef(String aRef) {
	}

	public String getDirectory() {
		return null;
	}

	public void setDirectory(String aDirectory) {
	}

	public String getFileName() {
		return null;
	}

	public void setFileName(String aFileName) {
	}

	public String getFileBaseName() {
		return null;
	}

	public void setFileBaseName(String aFileBaseName) {
	}

	public String getFileExtension() {
		return null;
	}

	public void setFileExtension(String aFileExtension) {
	}

	public String getCommonBaseSpec(nsIURI aURIToCompare) {
		return null;
	}

	public String getRelativeSpec(nsIURI aURIToCompare) {
		return null;
	}

	public String getSpec() {
		return null;
	}

	public void setSpec(String aSpec) {
	}

	public String getPrePath() {
		return null;
	}

	public String getScheme() {
		return null;
	}

	public void setScheme(String aScheme) {
	}

	public String getUserPass() {
		return null;
	}

	public void setUserPass(String aUserPass) {
	}

	public String getUsername() {
		return null;
	}

	public void setUsername(String aUsername) {
	}

	public String getPassword() {
		return null;
	}

	public void setPassword(String aPassword) {
	}

	public String getHostPort() {
		return null;
	}

	public void setHostPort(String aHostPort) {
	}

	public String getHost() {
		return null;
	}

	public void setHost(String aHost) {
	}

	public int getPort() {
		return 0;
	}

	public void setPort(int aPort) {
	}

	public String getPath() {
		return null;
	}

	public void setPath(String aPath) {
	}

	public boolean _equals(nsIURI other) {
		return false;
	}

	public boolean schemeIs(String scheme) {
		return false;
	}

	public nsIURI _clone() {
		return null;
	}

	public String resolve(String relativePath) {
		return null;
	}

	public String getAsciiSpec() {
		return null;
	}

	public String getAsciiHost() {
		return null;
	}

	public String getOriginCharset() {
		return null;
	}
}
