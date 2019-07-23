




























































#ifndef BASE_FIELD_TRIAL_H_
#define BASE_FIELD_TRIAL_H_

#include <map>
#include <string>

#include "base/lock.h"
#include "base/non_thread_safe.h"
#include "base/ref_counted.h"
#include "base/time.h"


class FieldTrial : public base::RefCounted<FieldTrial> {
 public:
  static const int kNotParticipating;

  typedef int Probability;  

  
  
  
  
  
  FieldTrial(const std::string& name, Probability total_probability);

  
  
  
  int AppendGroup(const std::string& name, Probability group_probability);

  
  std::string name() const { return name_; }

  
  
  
  int group() const { return group_; }

  
  
  
  std::string group_name() const { return group_name_; }

  
  
  static std::string MakeName(const std::string& name_prefix,
                              const std::string& trial_name);

 private:
  
  
  const std::string name_;

  
  
  Probability divisor_;

  
  
  
  Probability random_;

  
  Probability accumulated_group_probability_;

  int next_group_number_;

  
  
  int group_;

  
  
  std::string group_name_;

  DISALLOW_COPY_AND_ASSIGN(FieldTrial);
};





class FieldTrialList {
 public:
  
  FieldTrialList();
  
  ~FieldTrialList();

  
  
  static void Register(FieldTrial* trial);

  
  
  static FieldTrial* Find(const std::string& name);

  static int FindValue(const std::string& name);

  static std::string FindFullName(const std::string& name);

  
  
  
  
  
  static base::Time application_start_time() {
    if (global_)
      return global_->application_start_time_;
    
    return base::Time::Now();
  }

 private:
  
  FieldTrial* PreLockedFind(const std::string& name);

  typedef std::map<std::string, FieldTrial*> RegistrationList;

  static FieldTrialList* global_;  

  base::Time application_start_time_;

  
  Lock lock_;
  RegistrationList registered_;

  DISALLOW_COPY_AND_ASSIGN(FieldTrialList);
};

#endif  
