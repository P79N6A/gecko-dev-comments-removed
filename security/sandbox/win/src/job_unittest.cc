





#include "base/win/scoped_process_information.h"
#include "sandbox/win/src/job.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sandbox {


TEST(JobTest, TestCreation) {
  
  {
    
    Job job;
    ASSERT_EQ(ERROR_SUCCESS, job.Init(JOB_LOCKDOWN, L"my_test_job_name", 0, 0));

    
    HANDLE job_handle = ::OpenJobObjectW(GENERIC_ALL, FALSE,
                                         L"my_test_job_name");
    ASSERT_TRUE(job_handle != NULL);

    if (job_handle)
      CloseHandle(job_handle);
  }

  
  HANDLE job_handle = ::OpenJobObjectW(GENERIC_ALL, FALSE, L"my_test_job_name");
  ASSERT_TRUE(job_handle == NULL);
  ASSERT_EQ(ERROR_FILE_NOT_FOUND, ::GetLastError());
}


TEST(JobTest, TestDetach) {
  HANDLE job_handle;
  
  {
    
    Job job;
    ASSERT_EQ(ERROR_SUCCESS, job.Init(JOB_LOCKDOWN, L"my_test_job_name", 0, 0));

    job_handle = job.Detach();
    ASSERT_TRUE(job_handle != NULL);
  }

  
  
  HANDLE job_handle_dup = ::OpenJobObjectW(GENERIC_ALL, FALSE,
                                           L"my_test_job_name");
  ASSERT_TRUE(job_handle_dup != NULL);

  
  if (job_handle_dup)
    ::CloseHandle(job_handle_dup);

  if (job_handle)
    ::CloseHandle(job_handle);

  
  job_handle = ::OpenJobObjectW(GENERIC_ALL, FALSE, L"my_test_job_name");
  ASSERT_TRUE(job_handle == NULL);
  ASSERT_EQ(ERROR_FILE_NOT_FOUND, ::GetLastError());
}


TEST(JobTest, TestExceptions) {
  HANDLE job_handle;
  
  {
    
    Job job;
    ASSERT_EQ(ERROR_SUCCESS, job.Init(JOB_LOCKDOWN, L"my_test_job_name",
                                      JOB_OBJECT_UILIMIT_READCLIPBOARD, 0));

    job_handle = job.Detach();
    ASSERT_TRUE(job_handle != NULL);

    JOBOBJECT_BASIC_UI_RESTRICTIONS jbur = {0};
    DWORD size = sizeof(jbur);
    BOOL result = ::QueryInformationJobObject(job_handle,
                                              JobObjectBasicUIRestrictions,
                                              &jbur, size, &size);
    ASSERT_TRUE(result);

    ASSERT_EQ(jbur.UIRestrictionsClass & JOB_OBJECT_UILIMIT_READCLIPBOARD, 0);
    ::CloseHandle(job_handle);
  }

  
  {
    
    Job job;
    ASSERT_EQ(ERROR_SUCCESS, job.Init(JOB_LOCKDOWN, L"my_test_job_name", 0, 0));

    job_handle = job.Detach();
    ASSERT_TRUE(job_handle != NULL);

    JOBOBJECT_BASIC_UI_RESTRICTIONS jbur = {0};
    DWORD size = sizeof(jbur);
    BOOL result = ::QueryInformationJobObject(job_handle,
                                              JobObjectBasicUIRestrictions,
                                              &jbur, size, &size);
    ASSERT_TRUE(result);

    ASSERT_EQ(jbur.UIRestrictionsClass & JOB_OBJECT_UILIMIT_READCLIPBOARD,
              JOB_OBJECT_UILIMIT_READCLIPBOARD);
    ::CloseHandle(job_handle);
  }
}


TEST(JobTest, DoubleInit) {
  
  Job job;
  ASSERT_EQ(ERROR_SUCCESS, job.Init(JOB_LOCKDOWN, L"my_test_job_name", 0, 0));
  ASSERT_EQ(ERROR_ALREADY_INITIALIZED, job.Init(JOB_LOCKDOWN, L"test", 0, 0));
}



TEST(JobTest, NoInit) {
  Job job;
  ASSERT_EQ(ERROR_NO_DATA, job.UserHandleGrantAccess(NULL));
  ASSERT_EQ(ERROR_NO_DATA, job.AssignProcessToJob(NULL));
  ASSERT_TRUE(job.Detach() == NULL);
}


TEST(JobTest, SecurityLevel) {
  Job job1;
  ASSERT_EQ(ERROR_SUCCESS, job1.Init(JOB_LOCKDOWN, L"job1", 0, 0));

  Job job2;
  ASSERT_EQ(ERROR_SUCCESS, job2.Init(JOB_RESTRICTED, L"job2", 0, 0));

  Job job3;
  ASSERT_EQ(ERROR_SUCCESS, job3.Init(JOB_LIMITED_USER, L"job3", 0, 0));

  Job job4;
  ASSERT_EQ(ERROR_SUCCESS, job4.Init(JOB_INTERACTIVE, L"job4", 0, 0));

  Job job5;
  ASSERT_EQ(ERROR_SUCCESS, job5.Init(JOB_UNPROTECTED, L"job5", 0, 0));

  
  Job job6;
  ASSERT_EQ(ERROR_BAD_ARGUMENTS, job6.Init(JOB_NONE, L"job6", 0, 0));

  Job job7;
  ASSERT_EQ(ERROR_BAD_ARGUMENTS, job7.Init(
      static_cast<JobLevel>(JOB_NONE+1), L"job7", 0, 0));
}


TEST(JobTest, ProcessInJob) {
  
  Job job;
  ASSERT_EQ(ERROR_SUCCESS, job.Init(JOB_UNPROTECTED, L"job_test_process", 0,
                                    0));

  BOOL result = FALSE;

  wchar_t notepad[] = L"notepad";
  STARTUPINFO si = { sizeof(si) };
  PROCESS_INFORMATION temp_process_info = {};
  result = ::CreateProcess(NULL, notepad, NULL, NULL, FALSE, 0, NULL, NULL, &si,
                           &temp_process_info);
  ASSERT_TRUE(result);
  base::win::ScopedProcessInformation pi(temp_process_info);
  ASSERT_EQ(ERROR_SUCCESS, job.AssignProcessToJob(pi.process_handle()));

  
  HANDLE job_handle = job.Detach();

  
  JOBOBJECT_BASIC_PROCESS_ID_LIST jbpidl = {0};
  DWORD size = sizeof(jbpidl);
  result = ::QueryInformationJobObject(job_handle,
                                       JobObjectBasicProcessIdList,
                                       &jbpidl, size, &size);
  EXPECT_TRUE(result);

  EXPECT_EQ(1, jbpidl.NumberOfAssignedProcesses);
  EXPECT_EQ(1, jbpidl.NumberOfProcessIdsInList);
  EXPECT_EQ(pi.process_id(), jbpidl.ProcessIdList[0]);

  EXPECT_TRUE(::TerminateProcess(pi.process_handle(), 0));

  EXPECT_TRUE(::CloseHandle(job_handle));
}

}  
