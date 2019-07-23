



































#include "ScopedXREEmbed.h"

#include "base/command_line.h"
#include "base/string_util.h"

#include "nsIFile.h"
#include "nsILocalFile.h"

#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsStringGlue.h"
#include "nsXULAppAPI.h"

using mozilla::ipc::ScopedXREEmbed;

ScopedXREEmbed::ScopedXREEmbed()
: mShouldKillEmbedding(false)
{
  NS_LogInit();
}

ScopedXREEmbed::~ScopedXREEmbed()
{
  Stop();
  NS_LogTerm();
}

void
ScopedXREEmbed::Start()
{
  std::string path;
#if defined(OS_WIN)
  path = WideToUTF8(CommandLine::ForCurrentProcess()->program());
#elif defined(OS_POSIX)
  path = CommandLine::ForCurrentProcess()->argv()[0];
#else
#  error Sorry
#endif

  nsCOMPtr<nsILocalFile> localFile;
  nsresult rv = XRE_GetBinaryPath(path.c_str(), getter_AddRefs(localFile));
  NS_ENSURE_SUCCESS(rv,);

  nsCOMPtr<nsIFile> parent;
  rv = localFile->GetParent(getter_AddRefs(parent));
  NS_ENSURE_SUCCESS(rv,);

  localFile = do_QueryInterface(parent);
  NS_ENSURE_TRUE(localFile,);

  rv = XRE_InitEmbedding(localFile, localFile, nsnull, nsnull, 0);
  NS_ENSURE_SUCCESS(rv,);

  mShouldKillEmbedding = true;
}

void
ScopedXREEmbed::Stop()
{
  if (mShouldKillEmbedding) {
    XRE_TermEmbedding();
    mShouldKillEmbedding = false;
  }
}
