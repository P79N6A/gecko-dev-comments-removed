




































from optparse import OptionParser
import partner
import urllib


def main(branch, version, url, loc, name, vFiles, minDir, creds, aus):
  
  quotedurl = urllib.quote(urllib.unquote(url), ":/")
  partner.doDownload(name, loc, quotedurl, minDir, creds)
  partner.doInstall(branch, name, loc)
  partner.doMinotaur(name, loc, minDir, vFiles, aus, version)
  partner.doUninstall(branch, name, loc)

if __name__ == "__main__":
  parser = OptionParser()
  parser.add_option("-n", "--Name", dest="name", help="Build Name",
                    metavar="PARTNER_NAME")
  parser.add_option("-b", "--Branch", dest="branch",
                   help="Gecko Branch: 1.8.0|1.8.1|1.9", metavar="BRANCH")
  parser.add_option("-v", "--Version ", dest="version",
                    help="version of firefox to be tested",
                    metavar="FIREFOX_VERSION")
  parser.add_option("-u", "--UrlToBuild", dest="url",
                    help="URL to top level build location, above the OS directories",
                    metavar="URL")
  parser.add_option("-l", "--Locale", dest="loc", help="Locale for this build",
                    metavar="LOCALE")
  parser.add_option("-f", "--VerificationFileLocation", dest="verificationFiles",
                    help="location of verification files, leave blank to create verification files",
                    metavar="VER_FILES")
  parser.add_option("-m", "--MinotaurDirectory", dest="minDir",
                    help="Directory of the Minotuar code",
                    metavar="MINOTAUR_DIR")
  parser.add_option("-c", "--Credentials", dest="creds",
                    help="Credentials to download the build in this form: <user>:<password>",
                    metavar="CREDENTIALS")
  parser.add_option("-a", "--AusParameter", dest="aus",
                    help="The AUS parameter for the AUS URI (-cck param)",
                    metavar="AUS_PARAM")
  (options, args) = parser.parse_args()

  
  main(options.branch, options.version, options.url, options.loc, options.name,
       options.verificationFiles, options.minDir, options.creds, options.aus)
