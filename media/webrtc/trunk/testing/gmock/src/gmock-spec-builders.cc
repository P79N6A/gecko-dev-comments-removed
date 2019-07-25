



































#include "gmock/gmock-spec-builders.h"

#include <stdlib.h>
#include <iostream>  
#include <map>
#include <set>
#include <string>
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#if GTEST_OS_CYGWIN || GTEST_OS_LINUX || GTEST_OS_MAC
# include <unistd.h>  
#endif

namespace testing {
namespace internal {



GTEST_DEFINE_STATIC_MUTEX_(g_gmock_mutex);


void LogWithLocation(testing::internal::LogSeverity severity,
                     const char* file, int line,
                     const string& message) {
  ::std::ostringstream s;
  s << file << ":" << line << ": " << message << ::std::endl;
  Log(severity, s.str(), 0);
}


ExpectationBase::ExpectationBase(const char* a_file,
                                 int a_line,
                                 const string& a_source_text)
    : file_(a_file),
      line_(a_line),
      source_text_(a_source_text),
      cardinality_specified_(false),
      cardinality_(Exactly(1)),
      call_count_(0),
      retired_(false),
      extra_matcher_specified_(false),
      repeated_action_specified_(false),
      retires_on_saturation_(false),
      last_clause_(kNone),
      action_count_checked_(false) {}


ExpectationBase::~ExpectationBase() {}



void ExpectationBase::SpecifyCardinality(const Cardinality& a_cardinality) {
  cardinality_specified_ = true;
  cardinality_ = a_cardinality;
}


void ExpectationBase::RetireAllPreRequisites() {
  if (is_retired()) {
    
    
    return;
  }

  for (ExpectationSet::const_iterator it = immediate_prerequisites_.begin();
       it != immediate_prerequisites_.end(); ++it) {
    ExpectationBase* const prerequisite = it->expectation_base().get();
    if (!prerequisite->is_retired()) {
      prerequisite->RetireAllPreRequisites();
      prerequisite->Retire();
    }
  }
}




bool ExpectationBase::AllPrerequisitesAreSatisfied() const {
  g_gmock_mutex.AssertHeld();
  for (ExpectationSet::const_iterator it = immediate_prerequisites_.begin();
       it != immediate_prerequisites_.end(); ++it) {
    if (!(it->expectation_base()->IsSatisfied()) ||
        !(it->expectation_base()->AllPrerequisitesAreSatisfied()))
      return false;
  }
  return true;
}



void ExpectationBase::FindUnsatisfiedPrerequisites(
    ExpectationSet* result) const {
  g_gmock_mutex.AssertHeld();
  for (ExpectationSet::const_iterator it = immediate_prerequisites_.begin();
       it != immediate_prerequisites_.end(); ++it) {
    if (it->expectation_base()->IsSatisfied()) {
      
      
      if (it->expectation_base()->call_count_ == 0) {
        it->expectation_base()->FindUnsatisfiedPrerequisites(result);
      }
    } else {
      
      
      
      *result += *it;
    }
  }
}




void ExpectationBase::DescribeCallCountTo(::std::ostream* os) const {
  g_gmock_mutex.AssertHeld();

  
  *os << "         Expected: to be ";
  cardinality().DescribeTo(os);
  *os << "\n           Actual: ";
  Cardinality::DescribeActualCallCountTo(call_count(), os);

  
  
  *os << " - " << (IsOverSaturated() ? "over-saturated" :
                   IsSaturated() ? "saturated" :
                   IsSatisfied() ? "satisfied" : "unsatisfied")
      << " and "
      << (is_retired() ? "retired" : "active");
}






void ExpectationBase::CheckActionCountIfNotDone() const {
  bool should_check = false;
  {
    MutexLock l(&mutex_);
    if (!action_count_checked_) {
      action_count_checked_ = true;
      should_check = true;
    }
  }

  if (should_check) {
    if (!cardinality_specified_) {
      
      
      return;
    }

    
    const int action_count = static_cast<int>(untyped_actions_.size());
    const int upper_bound = cardinality().ConservativeUpperBound();
    const int lower_bound = cardinality().ConservativeLowerBound();
    bool too_many;  
    
    if (action_count > upper_bound ||
        (action_count == upper_bound && repeated_action_specified_)) {
      too_many = true;
    } else if (0 < action_count && action_count < lower_bound &&
               !repeated_action_specified_) {
      too_many = false;
    } else {
      return;
    }

    ::std::stringstream ss;
    DescribeLocationTo(&ss);
    ss << "Too " << (too_many ? "many" : "few")
       << " actions specified in " << source_text() << "...\n"
       << "Expected to be ";
    cardinality().DescribeTo(&ss);
    ss << ", but has " << (too_many ? "" : "only ")
       << action_count << " WillOnce()"
       << (action_count == 1 ? "" : "s");
    if (repeated_action_specified_) {
      ss << " and a WillRepeatedly()";
    }
    ss << ".";
    Log(WARNING, ss.str(), -1);  
  }
}


void ExpectationBase::UntypedTimes(const Cardinality& a_cardinality) {
  if (last_clause_ == kTimes) {
    ExpectSpecProperty(false,
                       ".Times() cannot appear "
                       "more than once in an EXPECT_CALL().");
  } else {
    ExpectSpecProperty(last_clause_ < kTimes,
                       ".Times() cannot appear after "
                       ".InSequence(), .WillOnce(), .WillRepeatedly(), "
                       "or .RetiresOnSaturation().");
  }
  last_clause_ = kTimes;

  SpecifyCardinality(a_cardinality);
}



ThreadLocal<Sequence*> g_gmock_implicit_sequence;



void ReportUninterestingCall(CallReaction reaction, const string& msg) {
  switch (reaction) {
    case ALLOW:
      Log(INFO, msg, 3);
      break;
    case WARN:
      Log(WARNING, msg, 3);
      break;
    default:  
      Expect(false, NULL, -1, msg);
  }
}

UntypedFunctionMockerBase::UntypedFunctionMockerBase()
    : mock_obj_(NULL), name_("") {}

UntypedFunctionMockerBase::~UntypedFunctionMockerBase() {}






void UntypedFunctionMockerBase::RegisterOwner(const void* mock_obj) {
  {
    MutexLock l(&g_gmock_mutex);
    mock_obj_ = mock_obj;
  }
  Mock::Register(mock_obj, this);
}





void UntypedFunctionMockerBase::SetOwnerAndName(
    const void* mock_obj, const char* name) {
  
  
  MutexLock l(&g_gmock_mutex);
  mock_obj_ = mock_obj;
  name_ = name;
}




const void* UntypedFunctionMockerBase::MockObject() const {
  const void* mock_obj;
  {
    
    
    MutexLock l(&g_gmock_mutex);
    Assert(mock_obj_ != NULL, __FILE__, __LINE__,
           "MockObject() must not be called before RegisterOwner() or "
           "SetOwnerAndName() has been called.");
    mock_obj = mock_obj_;
  }
  return mock_obj;
}




const char* UntypedFunctionMockerBase::Name() const {
  const char* name;
  {
    
    
    MutexLock l(&g_gmock_mutex);
    Assert(name_ != NULL, __FILE__, __LINE__,
           "Name() must not be called before SetOwnerAndName() has "
           "been called.");
    name = name_;
  }
  return name;
}





const UntypedActionResultHolderBase*
UntypedFunctionMockerBase::UntypedInvokeWith(const void* const untyped_args) {
  if (untyped_expectations_.size() == 0) {
    
    

    
    
    
    
    const CallReaction reaction =
        Mock::GetReactionOnUninterestingCalls(MockObject());

    
    
    
    const bool need_to_report_uninteresting_call =
        
        
        reaction == ALLOW ? LogIsVisible(INFO) :
        
        
        reaction == WARN ? LogIsVisible(WARNING) :
        
        
        true;

    if (!need_to_report_uninteresting_call) {
      
      return this->UntypedPerformDefaultAction(untyped_args, "");
    }

    
    ::std::stringstream ss;
    this->UntypedDescribeUninterestingCall(untyped_args, &ss);

    
    const UntypedActionResultHolderBase* const result =
        this->UntypedPerformDefaultAction(untyped_args, ss.str());

    
    if (result != NULL)
      result->PrintAsActionResult(&ss);

    ReportUninterestingCall(reaction, ss.str());
    return result;
  }

  bool is_excessive = false;
  ::std::stringstream ss;
  ::std::stringstream why;
  ::std::stringstream loc;
  const void* untyped_action = NULL;

  
  
  const ExpectationBase* const untyped_expectation =
      this->UntypedFindMatchingExpectation(
          untyped_args, &untyped_action, &is_excessive,
          &ss, &why);
  const bool found = untyped_expectation != NULL;

  
  
  
  const bool need_to_report_call = !found || is_excessive || LogIsVisible(INFO);
  if (!need_to_report_call) {
    
    return
        untyped_action == NULL ?
        this->UntypedPerformDefaultAction(untyped_args, "") :
        this->UntypedPerformAction(untyped_action, untyped_args);
  }

  ss << "    Function call: " << Name();
  this->UntypedPrintArgs(untyped_args, &ss);

  
  
  if (found && !is_excessive) {
    untyped_expectation->DescribeLocationTo(&loc);
  }

  const UntypedActionResultHolderBase* const result =
      untyped_action == NULL ?
      this->UntypedPerformDefaultAction(untyped_args, ss.str()) :
      this->UntypedPerformAction(untyped_action, untyped_args);
  if (result != NULL)
    result->PrintAsActionResult(&ss);
  ss << "\n" << why.str();

  if (!found) {
    
    Expect(false, NULL, -1, ss.str());
  } else if (is_excessive) {
    
    Expect(false, untyped_expectation->file(),
           untyped_expectation->line(), ss.str());
  } else {
    
    
    Log(INFO, loc.str() + ss.str(), 2);
  }

  return result;
}



Expectation UntypedFunctionMockerBase::GetHandleOf(ExpectationBase* exp) {
  for (UntypedExpectations::const_iterator it =
           untyped_expectations_.begin();
       it != untyped_expectations_.end(); ++it) {
    if (it->get() == exp) {
      return Expectation(*it);
    }
  }

  Assert(false, __FILE__, __LINE__, "Cannot find expectation.");
  return Expectation();
  
  
}





bool UntypedFunctionMockerBase::VerifyAndClearExpectationsLocked() {
  g_gmock_mutex.AssertHeld();
  bool expectations_met = true;
  for (UntypedExpectations::const_iterator it =
           untyped_expectations_.begin();
       it != untyped_expectations_.end(); ++it) {
    ExpectationBase* const untyped_expectation = it->get();
    if (untyped_expectation->IsOverSaturated()) {
      
      
      
      expectations_met = false;
    } else if (!untyped_expectation->IsSatisfied()) {
      expectations_met = false;
      ::std::stringstream ss;
      ss  << "Actual function call count doesn't match "
          << untyped_expectation->source_text() << "...\n";
      
      
      
      untyped_expectation->MaybeDescribeExtraMatcherTo(&ss);
      untyped_expectation->DescribeCallCountTo(&ss);
      Expect(false, untyped_expectation->file(),
             untyped_expectation->line(), ss.str());
    }
  }
  untyped_expectations_.clear();
  return expectations_met;
}

}  



namespace {

typedef std::set<internal::UntypedFunctionMockerBase*> FunctionMockers;




struct MockObjectState {
  MockObjectState()
      : first_used_file(NULL), first_used_line(-1), leakable(false) {}

  
  
  const char* first_used_file;
  int first_used_line;
  ::std::string first_used_test_case;
  ::std::string first_used_test;
  bool leakable;  
  FunctionMockers function_mockers;  
};





class MockObjectRegistry {
 public:
  
  typedef std::map<const void*, MockObjectState> StateMap;

  
  
  
  
  ~MockObjectRegistry() {
    
    

    if (!GMOCK_FLAG(catch_leaked_mocks))
      return;

    int leaked_count = 0;
    for (StateMap::const_iterator it = states_.begin(); it != states_.end();
         ++it) {
      if (it->second.leakable)  
        continue;

      
      
      std::cout << "\n";
      const MockObjectState& state = it->second;
      std::cout << internal::FormatFileLocation(state.first_used_file,
                                                state.first_used_line);
      std::cout << " ERROR: this mock object";
      if (state.first_used_test != "") {
        std::cout << " (used in test " << state.first_used_test_case << "."
             << state.first_used_test << ")";
      }
      std::cout << " should be deleted but never is. Its address is @"
           << it->first << ".";
      leaked_count++;
    }
    if (leaked_count > 0) {
      std::cout << "\nERROR: " << leaked_count
           << " leaked mock " << (leaked_count == 1 ? "object" : "objects")
           << " found at program exit.\n";
      std::cout.flush();
      ::std::cerr.flush();
      
      
      
      _exit(1);  
                 
    }
  }

  StateMap& states() { return states_; }
 private:
  StateMap states_;
};


MockObjectRegistry g_mock_object_registry;



std::map<const void*, internal::CallReaction> g_uninteresting_call_reaction;




void SetReactionOnUninterestingCalls(const void* mock_obj,
                                     internal::CallReaction reaction) {
  internal::MutexLock l(&internal::g_gmock_mutex);
  g_uninteresting_call_reaction[mock_obj] = reaction;
}

}  




void Mock::AllowUninterestingCalls(const void* mock_obj) {
  SetReactionOnUninterestingCalls(mock_obj, internal::ALLOW);
}




void Mock::WarnUninterestingCalls(const void* mock_obj) {
  SetReactionOnUninterestingCalls(mock_obj, internal::WARN);
}




void Mock::FailUninterestingCalls(const void* mock_obj) {
  SetReactionOnUninterestingCalls(mock_obj, internal::FAIL);
}




void Mock::UnregisterCallReaction(const void* mock_obj) {
  internal::MutexLock l(&internal::g_gmock_mutex);
  g_uninteresting_call_reaction.erase(mock_obj);
}




internal::CallReaction Mock::GetReactionOnUninterestingCalls(
    const void* mock_obj) {
  internal::MutexLock l(&internal::g_gmock_mutex);
  return (g_uninteresting_call_reaction.count(mock_obj) == 0) ?
      internal::WARN : g_uninteresting_call_reaction[mock_obj];
}




void Mock::AllowLeak(const void* mock_obj) {
  internal::MutexLock l(&internal::g_gmock_mutex);
  g_mock_object_registry.states()[mock_obj].leakable = true;
}





bool Mock::VerifyAndClearExpectations(void* mock_obj) {
  internal::MutexLock l(&internal::g_gmock_mutex);
  return VerifyAndClearExpectationsLocked(mock_obj);
}





bool Mock::VerifyAndClear(void* mock_obj) {
  internal::MutexLock l(&internal::g_gmock_mutex);
  ClearDefaultActionsLocked(mock_obj);
  return VerifyAndClearExpectationsLocked(mock_obj);
}





bool Mock::VerifyAndClearExpectationsLocked(void* mock_obj) {
  internal::g_gmock_mutex.AssertHeld();
  if (g_mock_object_registry.states().count(mock_obj) == 0) {
    
    return true;
  }

  
  
  bool expectations_met = true;
  FunctionMockers& mockers =
      g_mock_object_registry.states()[mock_obj].function_mockers;
  for (FunctionMockers::const_iterator it = mockers.begin();
       it != mockers.end(); ++it) {
    if (!(*it)->VerifyAndClearExpectationsLocked()) {
      expectations_met = false;
    }
  }

  
  
  return expectations_met;
}



void Mock::Register(const void* mock_obj,
                    internal::UntypedFunctionMockerBase* mocker) {
  internal::MutexLock l(&internal::g_gmock_mutex);
  g_mock_object_registry.states()[mock_obj].function_mockers.insert(mocker);
}





void Mock::RegisterUseByOnCallOrExpectCall(
    const void* mock_obj, const char* file, int line) {
  internal::MutexLock l(&internal::g_gmock_mutex);
  MockObjectState& state = g_mock_object_registry.states()[mock_obj];
  if (state.first_used_file == NULL) {
    state.first_used_file = file;
    state.first_used_line = line;
    const TestInfo* const test_info =
        UnitTest::GetInstance()->current_test_info();
    if (test_info != NULL) {
      
      
      
      state.first_used_test_case = test_info->test_case_name();
      state.first_used_test = test_info->name();
    }
  }
}






void Mock::UnregisterLocked(internal::UntypedFunctionMockerBase* mocker) {
  internal::g_gmock_mutex.AssertHeld();
  for (MockObjectRegistry::StateMap::iterator it =
           g_mock_object_registry.states().begin();
       it != g_mock_object_registry.states().end(); ++it) {
    FunctionMockers& mockers = it->second.function_mockers;
    if (mockers.erase(mocker) > 0) {
      
      if (mockers.empty()) {
        g_mock_object_registry.states().erase(it);
      }
      return;
    }
  }
}



void Mock::ClearDefaultActionsLocked(void* mock_obj) {
  internal::g_gmock_mutex.AssertHeld();

  if (g_mock_object_registry.states().count(mock_obj) == 0) {
    
    return;
  }

  
  
  FunctionMockers& mockers =
      g_mock_object_registry.states()[mock_obj].function_mockers;
  for (FunctionMockers::const_iterator it = mockers.begin();
       it != mockers.end(); ++it) {
    (*it)->ClearDefaultActionsLocked();
  }

  
  
}

Expectation::Expectation() {}

Expectation::Expectation(
    const internal::linked_ptr<internal::ExpectationBase>& an_expectation_base)
    : expectation_base_(an_expectation_base) {}

Expectation::~Expectation() {}


void Sequence::AddExpectation(const Expectation& expectation) const {
  if (*last_expectation_ != expectation) {
    if (last_expectation_->expectation_base() != NULL) {
      expectation.expectation_base()->immediate_prerequisites_
          += *last_expectation_;
    }
    *last_expectation_ = expectation;
  }
}


InSequence::InSequence() {
  if (internal::g_gmock_implicit_sequence.get() == NULL) {
    internal::g_gmock_implicit_sequence.set(new Sequence);
    sequence_created_ = true;
  } else {
    sequence_created_ = false;
  }
}



InSequence::~InSequence() {
  if (sequence_created_) {
    delete internal::g_gmock_implicit_sequence.get();
    internal::g_gmock_implicit_sequence.set(NULL);
  }
}

}  
