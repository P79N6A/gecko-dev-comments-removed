









#include "webrtc/video_engine/test/auto_test/interface/vie_autotest_main.h"

#include "gflags/gflags.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "webrtc/test/field_trial.h"
#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/video_engine/test/auto_test/interface/vie_autotest.h"
#include "webrtc/video_engine/test/auto_test/interface/vie_autotest_window_manager_interface.h"
#include "webrtc/video_engine/test/auto_test/interface/vie_window_creator.h"

DEFINE_bool(automated, false, "Run Video engine tests in noninteractive mode.");
DEFINE_bool(auto_custom_call, false, "Run custom call directly.");
DEFINE_string(force_fieldtrials, "",
    "Field trials control experimental feature code which can be forced. "
    "E.g. running with --force_fieldtrials=WebRTC-FooFeature/Enable/"
    " will assign the group Enable to field trial WebRTC-FooFeature.");

static const std::string kStandardTest = "ViEStandardIntegrationTest";
static const std::string kExtendedTest = "ViEExtendedIntegrationTest";
static const std::string kApiTest = "ViEApiIntegrationTest";

ViEAutoTestMain::ViEAutoTestMain() {
  index_to_test_method_map_[1] = "RunsBaseTestWithoutErrors";
  index_to_test_method_map_[2] = "RunsCaptureTestWithoutErrors";
  index_to_test_method_map_[3] = "RunsCodecTestWithoutErrors";
  index_to_test_method_map_[4] = "[unused]";
  index_to_test_method_map_[5] = "RunsImageProcessTestWithoutErrors";
  index_to_test_method_map_[6] = "RunsNetworkTestWithoutErrors";
  index_to_test_method_map_[7] = "RunsRenderTestWithoutErrors";
  index_to_test_method_map_[8] = "RunsRtpRtcpTestWithoutErrors";
}

int ViEAutoTestMain::RunTests(int argc, char** argv) {
  
  ViETest::Init();
  
  webrtc::test::SetExecutablePath(argv[0]);
  
  testing::InitGoogleTest(&argc, argv);
  
  
  google::AllowCommandLineReparsing();
  
  google::ParseCommandLineFlags(&argc, &argv, true);
  
  webrtc::test::InitFieldTrialsFromString(FLAGS_force_fieldtrials);

  int result;
  if (FLAGS_automated) {
    
#if defined(WEBRTC_LINUX)
    
    
    return 0;
#endif
    result = RUN_ALL_TESTS();
  } else if (FLAGS_auto_custom_call) {
    
    result = RunSpecialTestCase(8);
  } else {
    
    result = RunInteractiveMode();
  }

  ViETest::Terminate();
  return result;
}

int ViEAutoTestMain::AskUserForTestCase() {
  int choice;
  std::string answer;

  do {
    ViETest::Log("\nSpecific tests:");
    ViETest::Log("\t 0. Go back to previous menu.");

    
    int last_valid_choice = 0;
    std::map<int, std::string>::const_iterator iterator;
    for (iterator = index_to_test_method_map_.begin();
        iterator != index_to_test_method_map_.end();
        ++iterator) {
      ViETest::Log("\t %d. %s", iterator->first, iterator->second.c_str());
      last_valid_choice = iterator->first;
    }

    ViETest::Log("Choose specific test:");
    choice = AskUserForNumber(0, last_valid_choice);
  } while (choice == kInvalidChoice);

  return choice;
}

int ViEAutoTestMain::AskUserForNumber(int min_allowed, int max_allowed) {
  int result;
  if (scanf("%d", &result) <= 0) {
    ViETest::Log("\nPlease enter a number instead, then hit enter.");
    getc(stdin);
    return kInvalidChoice;
  }
  getc(stdin);  

  if (result < min_allowed || result > max_allowed) {
    ViETest::Log("%d-%d are valid choices. Please try again.", min_allowed,
                 max_allowed);
    return kInvalidChoice;
  }

  return result;
}

int ViEAutoTestMain::RunTestMatching(const std::string test_case,
                                     const std::string test_method) {
  testing::FLAGS_gtest_filter = test_case + "." + test_method;
  return RUN_ALL_TESTS();
}

int ViEAutoTestMain::RunSpecificTestCaseIn(const std::string test_case_name)
{
  
  int specific_choice = AskUserForTestCase();
  if (specific_choice != 0){
    return RunTestMatching(test_case_name,
                           index_to_test_method_map_[specific_choice]);
  }
  return 0;
}

int ViEAutoTestMain::RunSpecialTestCase(int choice) {
  
  assert(choice >= 7 && choice <= 10);

  
  ViEWindowCreator windowCreator;
  ViEAutoTestWindowManagerInterface* windowManager =
      windowCreator.CreateTwoWindows();

  
  ViEAutoTest vieAutoTest(windowManager->GetWindow1(),
                          windowManager->GetWindow2());

  int errors = 0;
  switch (choice) {
    case 7: errors = vieAutoTest.ViELoopbackCall();  break;
    case 8: errors = vieAutoTest.ViECustomCall();    break;
    case 9: errors = vieAutoTest.ViESimulcastCall(); break;
    case 10: errors = vieAutoTest.ViERecordCall();   break;
  }

  windowCreator.TerminateWindows();
  return errors;
}

int ViEAutoTestMain::RunInteractiveMode() {
  ViETest::Log(" ============================== ");
  ViETest::Log("    WebRTC ViE 3.x Autotest     ");
  ViETest::Log(" ============================== \n");

  int choice = 0;
  int errors = 0;
  do {
    ViETest::Log("Test types: ");
    ViETest::Log("\t 0. Quit");
    ViETest::Log("\t 1. All standard tests (delivery test)");
    ViETest::Log("\t 2. All API tests");
    ViETest::Log("\t 3. All extended test");
    ViETest::Log("\t 4. Specific standard test");
    ViETest::Log("\t 5. Specific API test");
    ViETest::Log("\t 6. Specific extended test");
    ViETest::Log("\t 7. Simple loopback call");
    ViETest::Log("\t 8. Custom configure a call");
    ViETest::Log("\t 9. Simulcast in loopback");
    ViETest::Log("\t 10. Record");
    ViETest::Log("Select type of test:");

    choice = AskUserForNumber(0, 10);
    if (choice == kInvalidChoice) {
      continue;
    }
    switch (choice) {
      case 0:                                                 break;
      case 1:  errors = RunTestMatching(kStandardTest, "*");  break;
      case 2:  errors = RunTestMatching(kApiTest,      "*");  break;
      case 3:  errors = RunTestMatching(kExtendedTest, "*");  break;
      case 4:  errors = RunSpecificTestCaseIn(kStandardTest); break;
      case 5:  errors = RunSpecificTestCaseIn(kApiTest);      break;
      case 6:  errors = RunSpecificTestCaseIn(kExtendedTest); break;
      default: errors = RunSpecialTestCase(choice);           break;
    }
  } while (choice != 0);

  if (errors) {
    ViETest::Log("Test done with errors, see ViEAutotestLog.txt for test "
        "result.\n");
    return 1;
  } else {
    ViETest::Log("Test done without errors, see ViEAutotestLog.txt for "
        "test result.\n");
    return 0;
  }
}
