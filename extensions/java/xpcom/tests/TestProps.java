



































import java.io.File;
import java.io.FileFilter;
import java.io.IOException;
import java.util.NoSuchElementException;
import java.util.Vector;

import org.mozilla.xpcom.Mozilla;
import org.mozilla.interfaces.nsIComponentManager;
import org.mozilla.interfaces.nsILocalFile;
import org.mozilla.interfaces.nsIProperties;
import org.mozilla.interfaces.nsIServiceManager;








public class TestProps {

	public static final String NS_PROPERTIES_CONTRACTID =
			"@mozilla.org/properties;1";

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

	private static void runTest() throws Exception {
		Mozilla mozilla = Mozilla.getInstance();
		nsIComponentManager componentManager = mozilla.getComponentManager();
		nsIProperties props = (nsIProperties) componentManager
				.createInstanceByContractID(NS_PROPERTIES_CONTRACTID, null,
						nsIProperties.NS_IPROPERTIES_IID);
		if (props == null) {
			throw new RuntimeException("Failed to create nsIProperties.");
		}

		
		nsILocalFile localFile1 = mozilla.newLocalFile("/user/local/share",
				false);
		nsILocalFile localFile2 = mozilla.newLocalFile("/home/foo", false);
		nsILocalFile localFile3 = mozilla.newLocalFile("/home/foo/bar", false);

		
		props.set("File One", localFile1);
		props.set("File Two", localFile2);
		props.set("File One Repeated", localFile1);
		props.set("File Three", localFile3);

		
		boolean hasProp = props.has("File One");
		if (hasProp == false)
			throw new NoSuchElementException("Could not find property " +
					"'File One'.");
		hasProp = props.has("File One Repeated");
		if (hasProp == false)
			throw new NoSuchElementException("Could not find property " +
					"'File One Repeated'.");
		hasProp = props.has("Nonexistant Property");
		if (hasProp == true)
			throw new Exception("Found property that doesn't exist.");

		
		nsILocalFile tempLocalFile = (nsILocalFile) props
				.get("File One Repeated", nsILocalFile.NS_ILOCALFILE_IID);
		if (tempLocalFile == null)
			throw new NoSuchElementException("Property 'File One Repeated' " +
					"not found.");
		if (tempLocalFile != localFile1)
			throw new Exception("Object returned by 'get' not the same as " +
					"object passed to 'set'.");

		
		hasProp = props.has("File Two");
		if (hasProp == false)
			throw new NoSuchElementException();
		props.undefine("File Two");
		hasProp = props.has("File Two");
		if (hasProp == true)
			throw new NoSuchElementException();

		
		long[] count = new long[1];
		String[] propKeys = props.getKeys(count);
		if (propKeys == null || propKeys.length != 3) {
			System.out.println("getKeys returned incorrect array.");
		}
		Vector activeKeys = new Vector(3);
		activeKeys.add("File One");
		activeKeys.add("File One Repeated");
		activeKeys.add("File Three");
		for (int i = 0; i < propKeys.length; i++) {
			if (!activeKeys.remove(propKeys[i])) {
				throw new RuntimeException("Found unknown key.");
			}
		}
	}
}
