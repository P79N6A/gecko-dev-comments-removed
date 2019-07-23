



#ifndef CHROME_BROWSER_COMMON_THUMBNAIL_SCORE_H__
#define CHROME_BROWSER_COMMON_THUMBNAIL_SCORE_H__

#include "base/time.h"


struct ThumbnailScore {
  
  
  ThumbnailScore();

  
  
  ThumbnailScore(double score, bool clipping, bool top);

  
  ThumbnailScore(double score, bool clipping, bool top,
                 const base::Time& time);
  ~ThumbnailScore();

  
  bool Equals(const ThumbnailScore& rhs) const;

  
  
  
  
  double boring_score;

  
  
  
  
  bool good_clipping;

  
  
  
  
  
  bool at_top;

  
  
  base::Time time_at_snapshot;

  
  static const double kThumbnailMaximumBoringness;

  
  
  
  static const base::TimeDelta kUpdateThumbnailTime;

  
  static const double kThumbnailDegradePerHour;
};


bool ShouldReplaceThumbnailWith(const ThumbnailScore& current,
                                const ThumbnailScore& replacement);

#endif  
