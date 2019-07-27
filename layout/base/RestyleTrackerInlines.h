




#ifndef mozilla_RestyleTrackerInlines_h
#define mozilla_RestyleTrackerInlines_h

#ifdef RESTYLE_LOGGING
bool
mozilla::RestyleTracker::ShouldLogRestyle()
{
  return mRestyleManager->ShouldLogRestyle();
}

int32_t&
mozilla::RestyleTracker::LoggingDepth()
{
  return mRestyleManager->LoggingDepth();
}
#endif

#endif 
