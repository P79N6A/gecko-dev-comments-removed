



































import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

import org.mozilla.xpcom.IAppFileLocProvider;


public class LocationProvider implements IAppFileLocProvider {

	private File libXULPath;
	private File profile;
	private File history;
	
	public LocationProvider(File aBinPath, File aProfileDir)
	throws IOException {
		libXULPath = aBinPath;
		profile = aProfileDir;

		if (!libXULPath.exists() || !libXULPath.isDirectory()) {
			throw new FileNotFoundException("libxul directory specified is not valid: "
					+ libXULPath.getAbsolutePath());
		}
		if (profile != null && (!profile.exists() || !profile.isDirectory())) {
			throw new FileNotFoundException("profile directory specified is not valid: "
					+ profile.getAbsolutePath());
		}

		
		if (profile != null) {
			setupProfile();
		}
	}
	
	private void setupProfile() throws IOException {
		history = new File(profile, "history.dat");
		if (!history.exists()) {
			history.createNewFile();
		}
	}

	public File getFile(String aProp, boolean[] aPersistent) {
		File file = null;
		if (aProp.equals("GreD") || aProp.equals("GreComsD")) {
			file = libXULPath;
			if (aProp.equals("GreComsD")) {
				file = new File(file, "components");
			}
		}
		else if (aProp.equals("MozBinD") || 
			aProp.equals("CurProcD") ||
			aProp.equals("ComsD")) 
		{
			file = libXULPath;
			if (aProp.equals("ComsD")) {
				file = new File(file, "components");
			}
		}
		else if (aProp.equals("ProfD")) {
			return profile;
		}
		else if (aProp.equals("UHist")) {
			return history;
		}

		return file;
	}

	public File[] getFiles(String aProp) {
		File[] files = null;
		if (aProp.equals("APluginsDL")) {
			files = new File[1];
			files[0] = new File(libXULPath, "plugins");
		}

		return files;
	}

}
