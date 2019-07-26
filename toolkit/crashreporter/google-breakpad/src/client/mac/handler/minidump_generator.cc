




























#include <algorithm>
#include <cstdio>

#include <mach/host_info.h>
#include <mach/machine.h>
#include <mach/vm_statistics.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <sys/sysctl.h>
#include <sys/resource.h>

#include <CoreFoundation/CoreFoundation.h>

#include "client/mac/handler/minidump_generator.h"

#ifdef HAS_ARM_SUPPORT
#include <mach/arm/thread_status.h>
#endif
#ifdef HAS_PPC_SUPPORT
#include <mach/ppc/thread_status.h>
#endif
#ifdef HAS_X86_SUPPORT
#include <mach/i386/thread_status.h>
#endif

#include "client/minidump_file_writer-inl.h"
#include "common/mac/file_id.h"
#include "common/mac/macho_id.h"
#include "common/mac/string_utilities.h"

using MacStringUtils::ConvertToString;
using MacStringUtils::IntegerValueAtIndex;

namespace google_breakpad {

#if __LP64__
#define LC_SEGMENT_ARCH LC_SEGMENT_64
#else
#define LC_SEGMENT_ARCH LC_SEGMENT
#endif


MinidumpGenerator::MinidumpGenerator()
    : writer_(),
      exception_type_(0),
      exception_code_(0),
      exception_subcode_(0),
      exception_thread_(0),
      crashing_task_(mach_task_self()),
      handler_thread_(mach_thread_self()),
      cpu_type_(DynamicImages::GetNativeCPUType()),
      task_context_(NULL),
      dynamic_images_(NULL),
      memory_blocks_(&allocator_) {
  GatherSystemInformation();
}



MinidumpGenerator::MinidumpGenerator(mach_port_t crashing_task,
                                     mach_port_t handler_thread)
    : writer_(),
      exception_type_(0),
      exception_code_(0),
      exception_subcode_(0),
      exception_thread_(0),
      crashing_task_(crashing_task),
      handler_thread_(handler_thread),
      cpu_type_(DynamicImages::GetNativeCPUType()),
      task_context_(NULL),
      dynamic_images_(NULL),
      memory_blocks_(&allocator_) {
  if (crashing_task != mach_task_self()) {
    dynamic_images_ = new DynamicImages(crashing_task_);
    cpu_type_ = dynamic_images_->GetCPUType();
  } else {
    dynamic_images_ = NULL;
    cpu_type_ = DynamicImages::GetNativeCPUType();
  }

  GatherSystemInformation();
}

MinidumpGenerator::~MinidumpGenerator() {
  delete dynamic_images_;
}

char MinidumpGenerator::build_string_[16];
int MinidumpGenerator::os_major_version_ = 0;
int MinidumpGenerator::os_minor_version_ = 0;
int MinidumpGenerator::os_build_number_ = 0;


void MinidumpGenerator::GatherSystemInformation() {
  
  if (os_major_version_)
    return;

  
  CFStringRef vers_path =
    CFSTR("/System/Library/CoreServices/SystemVersion.plist");
  CFURLRef sys_vers =
    CFURLCreateWithFileSystemPath(NULL,
                                  vers_path,
                                  kCFURLPOSIXPathStyle,
                                  false);
  CFDataRef data;
  SInt32 error;
  CFURLCreateDataAndPropertiesFromResource(NULL, sys_vers, &data, NULL, NULL,
                                           &error);

  if (!data) {
    CFRelease(sys_vers);
    return;
  }

  CFDictionaryRef list = static_cast<CFDictionaryRef>
    (CFPropertyListCreateFromXMLData(NULL, data, kCFPropertyListImmutable,
                                     NULL));
  if (!list) {
    CFRelease(sys_vers);
    CFRelease(data);
    return;
  }

  CFStringRef build_version = static_cast<CFStringRef>
    (CFDictionaryGetValue(list, CFSTR("ProductBuildVersion")));
  CFStringRef product_version = static_cast<CFStringRef>
    (CFDictionaryGetValue(list, CFSTR("ProductVersion")));
  string build_str = ConvertToString(build_version);
  string product_str = ConvertToString(product_version);

  CFRelease(list);
  CFRelease(sys_vers);
  CFRelease(data);

  strlcpy(build_string_, build_str.c_str(), sizeof(build_string_));

  
  os_major_version_ = IntegerValueAtIndex(product_str, 0);
  os_minor_version_ = IntegerValueAtIndex(product_str, 1);
  os_build_number_ = IntegerValueAtIndex(product_str, 2);
}

void MinidumpGenerator::SetTaskContext(ucontext_t *task_context) {
  task_context_ = task_context;
}

string MinidumpGenerator::UniqueNameInDirectory(const string &dir,
                                                string *unique_name) {
  CFUUIDRef uuid = CFUUIDCreate(NULL);
  CFStringRef uuid_cfstr = CFUUIDCreateString(NULL, uuid);
  CFRelease(uuid);
  string file_name(ConvertToString(uuid_cfstr));
  CFRelease(uuid_cfstr);
  string path(dir);

  
  
  if (!dir.empty()) {
    if (dir.at(dir.size() - 1) != '/')
      path.append(1, '/');
  }

  path.append(file_name);
  path.append(".dmp");

  if (unique_name)
    *unique_name = file_name;

  return path;
}

bool MinidumpGenerator::Write(const char *path) {
  WriteStreamFN writers[] = {
    &MinidumpGenerator::WriteThreadListStream,
    &MinidumpGenerator::WriteMemoryListStream,
    &MinidumpGenerator::WriteSystemInfoStream,
    &MinidumpGenerator::WriteModuleListStream,
    &MinidumpGenerator::WriteMiscInfoStream,
    &MinidumpGenerator::WriteBreakpadInfoStream,
    
    
    
    &MinidumpGenerator::WriteExceptionStream,
  };
  bool result = false;

  
  
  
  if (writer_.Open(path)) {
    TypedMDRVA<MDRawHeader> header(&writer_);
    TypedMDRVA<MDRawDirectory> dir(&writer_);

    if (!header.Allocate())
      return false;

    int writer_count = static_cast<int>(sizeof(writers) / sizeof(writers[0]));

    
    
    if (!exception_thread_ && !exception_type_)
      --writer_count;

    
    if (!dir.AllocateArray(writer_count))
      return false;

    MDRawHeader *header_ptr = header.get();
    header_ptr->signature = MD_HEADER_SIGNATURE;
    header_ptr->version = MD_HEADER_VERSION;
    time(reinterpret_cast<time_t *>(&(header_ptr->time_date_stamp)));
    header_ptr->stream_count = writer_count;
    header_ptr->stream_directory_rva = dir.position();

    MDRawDirectory local_dir;
    result = true;
    for (int i = 0; (result) && (i < writer_count); ++i) {
      result = (this->*writers[i])(&local_dir);

      if (result)
        dir.CopyIndex(i, &local_dir);
    }
  }
  return result;
}

size_t MinidumpGenerator::CalculateStackSize(mach_vm_address_t start_addr) {
  mach_vm_address_t stack_region_base = start_addr;
  mach_vm_size_t stack_region_size;
  natural_t nesting_level = 0;
  vm_region_submap_info_64 submap_info;
  mach_msg_type_number_t info_count = VM_REGION_SUBMAP_INFO_COUNT_64;

  vm_region_recurse_info_t region_info;
  region_info = reinterpret_cast<vm_region_recurse_info_t>(&submap_info);

  if (start_addr == 0) {
    return 0;
  }

  kern_return_t result =
    mach_vm_region_recurse(crashing_task_, &stack_region_base,
                           &stack_region_size, &nesting_level,
                           region_info, &info_count);

  if (result != KERN_SUCCESS || start_addr < stack_region_base) {
    
    
    return 0;
  }

  unsigned int tag = submap_info.user_tag;

  
  
  
  
  
  
  if (tag == VM_MEMORY_STACK) {
    while (true) {
      mach_vm_address_t next_region_base = stack_region_base +
                                           stack_region_size;
      mach_vm_address_t proposed_next_region_base = next_region_base;
      mach_vm_size_t next_region_size;
      nesting_level = 0;
      info_count = VM_REGION_SUBMAP_INFO_COUNT_64;
      result = mach_vm_region_recurse(crashing_task_, &next_region_base,
                                      &next_region_size, &nesting_level,
                                      region_info, &info_count);
      if (result != KERN_SUCCESS ||
          next_region_base != proposed_next_region_base ||
          submap_info.user_tag != tag ||
          (submap_info.protection & VM_PROT_READ) == 0) {
        break;
      }

      stack_region_size += next_region_size;
    }
  }

  return stack_region_base + stack_region_size - start_addr;
}

bool MinidumpGenerator::WriteStackFromStartAddress(
    mach_vm_address_t start_addr,
    MDMemoryDescriptor *stack_location) {
  UntypedMDRVA memory(&writer_);

  bool result = false;
  size_t size = CalculateStackSize(start_addr);

  if (size == 0) {
      
      
      
      start_addr = 0xDEADBEEF;
      size = 16;
      if (!memory.Allocate(size))
        return false;

      unsigned long long dummy_stack[2];  
                                          
      dummy_stack[0] = 0xDEADBEEF;
      dummy_stack[1] = 0xDEADBEEF;

      result = memory.Copy(dummy_stack, size);
  } else {

    if (!memory.Allocate(size))
      return false;

    if (dynamic_images_) {
      vector<uint8_t> stack_memory;
      if (ReadTaskMemory(crashing_task_,
                         start_addr,
                         size,
                         stack_memory) != KERN_SUCCESS) {
        return false;
      }

      result = memory.Copy(&stack_memory[0], size);
    } else {
      result = memory.Copy(reinterpret_cast<const void *>(start_addr), size);
    }
  }

  stack_location->start_of_memory_range = start_addr;
  stack_location->memory = memory.location();

  return result;
}

bool MinidumpGenerator::WriteStack(breakpad_thread_state_data_t state,
                                   MDMemoryDescriptor *stack_location) {
  switch (cpu_type_) {
#ifdef HAS_ARM_SUPPORT
    case CPU_TYPE_ARM:
      return WriteStackARM(state, stack_location);
#endif
#ifdef HAS_PPC_SUPPORT
    case CPU_TYPE_POWERPC:
      return WriteStackPPC(state, stack_location);
    case CPU_TYPE_POWERPC64:
      return WriteStackPPC64(state, stack_location);
#endif
#ifdef HAS_X86_SUPPORT
    case CPU_TYPE_I386:
      return WriteStackX86(state, stack_location);
    case CPU_TYPE_X86_64:
      return WriteStackX86_64(state, stack_location);
#endif
    default:
      return false;
  }
}

bool MinidumpGenerator::WriteContext(breakpad_thread_state_data_t state,
                                     MDLocationDescriptor *register_location) {
  switch (cpu_type_) {
#ifdef HAS_ARM_SUPPORT
    case CPU_TYPE_ARM:
      return WriteContextARM(state, register_location);
#endif
#ifdef HAS_PPC_SUPPORT
    case CPU_TYPE_POWERPC:
      return WriteContextPPC(state, register_location);
    case CPU_TYPE_POWERPC64:
      return WriteContextPPC64(state, register_location);
#endif
#ifdef HAS_X86_SUPPORT
    case CPU_TYPE_I386:
      return WriteContextX86(state, register_location);
    case CPU_TYPE_X86_64:
      return WriteContextX86_64(state, register_location);
#endif
    default:
      return false;
  }
}

uint64_t MinidumpGenerator::CurrentPCForStack(
    breakpad_thread_state_data_t state) {
  switch (cpu_type_) {
#ifdef HAS_ARM_SUPPORT
    case CPU_TYPE_ARM:
      return CurrentPCForStackARM(state);
#endif
#ifdef HAS_PPC_SUPPORT
    case CPU_TYPE_POWERPC:
      return CurrentPCForStackPPC(state);
    case CPU_TYPE_POWERPC64:
      return CurrentPCForStackPPC64(state);
#endif
#ifdef HAS_X86_SUPPORT
    case CPU_TYPE_I386:
      return CurrentPCForStackX86(state);
    case CPU_TYPE_X86_64:
      return CurrentPCForStackX86_64(state);
#endif
    default:
      assert(0 && "Unknown CPU type!");
      return 0;
  }
}

#ifdef HAS_ARM_SUPPORT
bool MinidumpGenerator::WriteStackARM(breakpad_thread_state_data_t state,
                                      MDMemoryDescriptor *stack_location) {
  arm_thread_state_t *machine_state =
      reinterpret_cast<arm_thread_state_t *>(state);
  mach_vm_address_t start_addr = REGISTER_FROM_THREADSTATE(machine_state, sp);
  return WriteStackFromStartAddress(start_addr, stack_location);
}

uint64_t
MinidumpGenerator::CurrentPCForStackARM(breakpad_thread_state_data_t state) {
  arm_thread_state_t *machine_state =
      reinterpret_cast<arm_thread_state_t *>(state);

  return REGISTER_FROM_THREADSTATE(machine_state, pc);
}

bool MinidumpGenerator::WriteContextARM(breakpad_thread_state_data_t state,
                                        MDLocationDescriptor *register_location)
{
  TypedMDRVA<MDRawContextARM> context(&writer_);
  arm_thread_state_t *machine_state =
      reinterpret_cast<arm_thread_state_t *>(state);

  if (!context.Allocate())
    return false;

  *register_location = context.location();
  MDRawContextARM *context_ptr = context.get();
  context_ptr->context_flags = MD_CONTEXT_ARM_FULL;

#define AddGPR(a) context_ptr->iregs[a] = REGISTER_FROM_THREADSTATE(machine_state, r[a])

  context_ptr->iregs[13] = REGISTER_FROM_THREADSTATE(machine_state, sp);
  context_ptr->iregs[14] = REGISTER_FROM_THREADSTATE(machine_state, lr);
  context_ptr->iregs[15] = REGISTER_FROM_THREADSTATE(machine_state, pc);
  context_ptr->cpsr = REGISTER_FROM_THREADSTATE(machine_state, cpsr);

  AddGPR(0);
  AddGPR(1);
  AddGPR(2);
  AddGPR(3);
  AddGPR(4);
  AddGPR(5);
  AddGPR(6);
  AddGPR(7);
  AddGPR(8);
  AddGPR(9);
  AddGPR(10);
  AddGPR(11);
  AddGPR(12);
#undef AddReg
#undef AddGPR

  return true;
}
#endif

#ifdef HAS_PCC_SUPPORT
bool MinidumpGenerator::WriteStackPPC(breakpad_thread_state_data_t state,
                                      MDMemoryDescriptor *stack_location) {
  ppc_thread_state_t *machine_state =
      reinterpret_cast<ppc_thread_state_t *>(state);
  mach_vm_address_t start_addr = REGISTER_FROM_THREADSTATE(machine_state, r1);
  return WriteStackFromStartAddress(start_addr, stack_location);
}

bool MinidumpGenerator::WriteStackPPC64(breakpad_thread_state_data_t state,
                                        MDMemoryDescriptor *stack_location) {
  ppc_thread_state64_t *machine_state =
      reinterpret_cast<ppc_thread_state64_t *>(state);
  mach_vm_address_t start_addr = REGISTER_FROM_THREADSTATE(machine_state, r1);
  return WriteStackFromStartAddress(start_addr, stack_location);
}

uint64_t
MinidumpGenerator::CurrentPCForStackPPC(breakpad_thread_state_data_t state) {
  ppc_thread_state_t *machine_state =
      reinterpret_cast<ppc_thread_state_t *>(state);

  return REGISTER_FROM_THREADSTATE(machine_state, srr0);
}

uint64_t
MinidumpGenerator::CurrentPCForStackPPC64(breakpad_thread_state_data_t state) {
  ppc_thread_state64_t *machine_state =
      reinterpret_cast<ppc_thread_state64_t *>(state);

  return REGISTER_FROM_THREADSTATE(machine_state, srr0);
}

bool MinidumpGenerator::WriteContextPPC(breakpad_thread_state_data_t state,
                                        MDLocationDescriptor *register_location)
{
  TypedMDRVA<MDRawContextPPC> context(&writer_);
  ppc_thread_state_t *machine_state =
      reinterpret_cast<ppc_thread_state_t *>(state);

  if (!context.Allocate())
    return false;

  *register_location = context.location();
  MDRawContextPPC *context_ptr = context.get();
  context_ptr->context_flags = MD_CONTEXT_PPC_BASE;

#define AddReg(a) context_ptr->a = REGISTER_FROM_THREADSTATE(machine_state, a)
#define AddGPR(a) context_ptr->gpr[a] = REGISTER_FROM_THREADSTATE(machine_state, r ## a)

  AddReg(srr0);
  AddReg(cr);
  AddReg(xer);
  AddReg(ctr);
  AddReg(lr);
  AddReg(vrsave);

  AddGPR(0);
  AddGPR(1);
  AddGPR(2);
  AddGPR(3);
  AddGPR(4);
  AddGPR(5);
  AddGPR(6);
  AddGPR(7);
  AddGPR(8);
  AddGPR(9);
  AddGPR(10);
  AddGPR(11);
  AddGPR(12);
  AddGPR(13);
  AddGPR(14);
  AddGPR(15);
  AddGPR(16);
  AddGPR(17);
  AddGPR(18);
  AddGPR(19);
  AddGPR(20);
  AddGPR(21);
  AddGPR(22);
  AddGPR(23);
  AddGPR(24);
  AddGPR(25);
  AddGPR(26);
  AddGPR(27);
  AddGPR(28);
  AddGPR(29);
  AddGPR(30);
  AddGPR(31);
  AddReg(mq);
#undef AddReg
#undef AddGPR

  return true;
}

bool MinidumpGenerator::WriteContextPPC64(
    breakpad_thread_state_data_t state,
    MDLocationDescriptor *register_location) {
  TypedMDRVA<MDRawContextPPC64> context(&writer_);
  ppc_thread_state64_t *machine_state =
      reinterpret_cast<ppc_thread_state64_t *>(state);

  if (!context.Allocate())
    return false;

  *register_location = context.location();
  MDRawContextPPC64 *context_ptr = context.get();
  context_ptr->context_flags = MD_CONTEXT_PPC_BASE;

#define AddReg(a) context_ptr->a = REGISTER_FROM_THREADSTATE(machine_state, a)
#define AddGPR(a) context_ptr->gpr[a] = REGISTER_FROM_THREADSTATE(machine_state, r ## a)

  AddReg(srr0);
  AddReg(cr);
  AddReg(xer);
  AddReg(ctr);
  AddReg(lr);
  AddReg(vrsave);

  AddGPR(0);
  AddGPR(1);
  AddGPR(2);
  AddGPR(3);
  AddGPR(4);
  AddGPR(5);
  AddGPR(6);
  AddGPR(7);
  AddGPR(8);
  AddGPR(9);
  AddGPR(10);
  AddGPR(11);
  AddGPR(12);
  AddGPR(13);
  AddGPR(14);
  AddGPR(15);
  AddGPR(16);
  AddGPR(17);
  AddGPR(18);
  AddGPR(19);
  AddGPR(20);
  AddGPR(21);
  AddGPR(22);
  AddGPR(23);
  AddGPR(24);
  AddGPR(25);
  AddGPR(26);
  AddGPR(27);
  AddGPR(28);
  AddGPR(29);
  AddGPR(30);
  AddGPR(31);
#undef AddReg
#undef AddGPR

  return true;
}

#endif

#ifdef HAS_X86_SUPPORT
bool MinidumpGenerator::WriteStackX86(breakpad_thread_state_data_t state,
                                   MDMemoryDescriptor *stack_location) {
  i386_thread_state_t *machine_state =
      reinterpret_cast<i386_thread_state_t *>(state);

  mach_vm_address_t start_addr = REGISTER_FROM_THREADSTATE(machine_state, esp);
  return WriteStackFromStartAddress(start_addr, stack_location);
}

bool MinidumpGenerator::WriteStackX86_64(breakpad_thread_state_data_t state,
                                         MDMemoryDescriptor *stack_location) {
  x86_thread_state64_t *machine_state =
      reinterpret_cast<x86_thread_state64_t *>(state);

  mach_vm_address_t start_addr = REGISTER_FROM_THREADSTATE(machine_state, rsp);
  return WriteStackFromStartAddress(start_addr, stack_location);
}

uint64_t
MinidumpGenerator::CurrentPCForStackX86(breakpad_thread_state_data_t state) {
  i386_thread_state_t *machine_state =
      reinterpret_cast<i386_thread_state_t *>(state);

  return REGISTER_FROM_THREADSTATE(machine_state, eip);
}

uint64_t
MinidumpGenerator::CurrentPCForStackX86_64(breakpad_thread_state_data_t state) {
  x86_thread_state64_t *machine_state =
      reinterpret_cast<x86_thread_state64_t *>(state);

  return REGISTER_FROM_THREADSTATE(machine_state, rip);
}

bool MinidumpGenerator::WriteContextX86(breakpad_thread_state_data_t state,
                                        MDLocationDescriptor *register_location)
{
  TypedMDRVA<MDRawContextX86> context(&writer_);
  i386_thread_state_t *machine_state =
      reinterpret_cast<i386_thread_state_t *>(state);

  if (!context.Allocate())
    return false;

  *register_location = context.location();
  MDRawContextX86 *context_ptr = context.get();

#define AddReg(a) context_ptr->a = REGISTER_FROM_THREADSTATE(machine_state, a)

  context_ptr->context_flags = MD_CONTEXT_X86;
  AddReg(eax);
  AddReg(ebx);
  AddReg(ecx);
  AddReg(edx);
  AddReg(esi);
  AddReg(edi);
  AddReg(ebp);
  AddReg(esp);

  AddReg(cs);
  AddReg(ds);
  AddReg(ss);
  AddReg(es);
  AddReg(fs);
  AddReg(gs);
  AddReg(eflags);

  AddReg(eip);
#undef AddReg

  return true;
}

bool MinidumpGenerator::WriteContextX86_64(
    breakpad_thread_state_data_t state,
    MDLocationDescriptor *register_location) {
  TypedMDRVA<MDRawContextAMD64> context(&writer_);
  x86_thread_state64_t *machine_state =
      reinterpret_cast<x86_thread_state64_t *>(state);

  if (!context.Allocate())
    return false;

  *register_location = context.location();
  MDRawContextAMD64 *context_ptr = context.get();

#define AddReg(a) context_ptr->a = REGISTER_FROM_THREADSTATE(machine_state, a)

  context_ptr->context_flags = MD_CONTEXT_AMD64;
  AddReg(rax);
  AddReg(rbx);
  AddReg(rcx);
  AddReg(rdx);
  AddReg(rdi);
  AddReg(rsi);
  AddReg(rbp);
  AddReg(rsp);
  AddReg(r8);
  AddReg(r9);
  AddReg(r10);
  AddReg(r11);
  AddReg(r12);
  AddReg(r13);
  AddReg(r14);
  AddReg(r15);
  AddReg(rip);
  
  
  
  
  context_ptr->eflags = static_cast<uint32_t>(REGISTER_FROM_THREADSTATE(machine_state, rflags));
  AddReg(cs);
  AddReg(fs);
  AddReg(gs);
#undef AddReg

  return true;
}
#endif

bool MinidumpGenerator::GetThreadState(thread_act_t target_thread,
                                       thread_state_t state,
                                       mach_msg_type_number_t *count) {
  if (task_context_ && target_thread == mach_thread_self()) {
    switch (cpu_type_) {
#ifdef HAS_ARM_SUPPORT
      case CPU_TYPE_ARM: {
        size_t final_size =
            std::min(static_cast<size_t>(*count), sizeof(arm_thread_state_t));
        memcpy(state, &task_context_->uc_mcontext->__ss, final_size);
        *count = final_size;
        return true;
      }
#endif
#ifdef HAS_X86_SUPPORT
    case CPU_TYPE_I386:
    case CPU_TYPE_X86_64: {
        size_t state_size = cpu_type_ == CPU_TYPE_I386 ?
            sizeof(i386_thread_state_t) : sizeof(x86_thread_state64_t);
        size_t final_size =
            std::min(static_cast<size_t>(*count), state_size);
        memcpy(state, &task_context_->uc_mcontext->__ss, final_size);
        *count = final_size;
        return true;
      }
#endif
    }
  }

  thread_state_flavor_t flavor;
  switch (cpu_type_) {
#ifdef HAS_ARM_SUPPORT
    case CPU_TYPE_ARM:
      flavor = ARM_THREAD_STATE;
      break;
#endif
#ifdef HAS_PPC_SUPPORT
    case CPU_TYPE_POWERPC:
      flavor = PPC_THREAD_STATE;
      break;
    case CPU_TYPE_POWERPC64:
      flavor = PPC_THREAD_STATE64;
      break;
#endif
#ifdef HAS_X86_SUPPORT
    case CPU_TYPE_I386:
      flavor = i386_THREAD_STATE;
      break;
    case CPU_TYPE_X86_64:
      flavor = x86_THREAD_STATE64;
      break;
#endif
    default:
      return false;
  }
  return thread_get_state(target_thread, flavor,
                          state, count) == KERN_SUCCESS;
}

bool MinidumpGenerator::WriteThreadStream(mach_port_t thread_id,
                                          MDRawThread *thread) {
  breakpad_thread_state_data_t state;
  mach_msg_type_number_t state_count
      = static_cast<mach_msg_type_number_t>(sizeof(state));

  if (GetThreadState(thread_id, state, &state_count)) {
    if (!WriteStack(state, &thread->stack))
      return false;

    memory_blocks_.push_back(thread->stack);

    if (!WriteContext(state, &thread->thread_context))
      return false;

    thread->thread_id = thread_id;
  } else {
    return false;
  }

  return true;
}

bool MinidumpGenerator::WriteThreadListStream(
    MDRawDirectory *thread_list_stream) {
  TypedMDRVA<MDRawThreadList> list(&writer_);
  thread_act_port_array_t threads_for_task;
  mach_msg_type_number_t thread_count;
  int non_generator_thread_count;

  if (task_threads(crashing_task_, &threads_for_task, &thread_count))
    return false;

  
  if (handler_thread_ != MACH_PORT_NULL)
    non_generator_thread_count = thread_count - 1;
  else
    non_generator_thread_count = thread_count;
  if (!list.AllocateObjectAndArray(non_generator_thread_count,
                                   sizeof(MDRawThread)))
    return false;

  thread_list_stream->stream_type = MD_THREAD_LIST_STREAM;
  thread_list_stream->location = list.location();

  list.get()->number_of_threads = non_generator_thread_count;

  MDRawThread thread;
  int thread_idx = 0;

  for (unsigned int i = 0; i < thread_count; ++i) {
    memset(&thread, 0, sizeof(MDRawThread));

    if (threads_for_task[i] != handler_thread_) {
      if (!WriteThreadStream(threads_for_task[i], &thread))
        return false;

      list.CopyIndexAfterObject(thread_idx++, &thread, sizeof(MDRawThread));
    }
  }

  return true;
}

bool MinidumpGenerator::WriteMemoryListStream(
    MDRawDirectory *memory_list_stream) {
  TypedMDRVA<MDRawMemoryList> list(&writer_);

  
  
  const size_t kIPMemorySize = 256;  
  bool have_ip_memory = false;
  MDMemoryDescriptor ip_memory_d;
  if (exception_thread_ && exception_type_) {
    breakpad_thread_state_data_t state;
    mach_msg_type_number_t stateCount
      = static_cast<mach_msg_type_number_t>(sizeof(state));

    if (GetThreadState(exception_thread_, state, &stateCount)) {
      uint64_t ip = CurrentPCForStack(state);
      
      
      
      mach_vm_address_t addr = static_cast<vm_address_t>(ip);
      mach_vm_size_t size;
      natural_t nesting_level = 0;
      vm_region_submap_info_64 info;
      mach_msg_type_number_t info_count = VM_REGION_SUBMAP_INFO_COUNT_64;
      vm_region_recurse_info_t recurse_info;
      recurse_info = reinterpret_cast<vm_region_recurse_info_t>(&info);

      kern_return_t ret =
        mach_vm_region_recurse(crashing_task_,
                               &addr,
                               &size,
                               &nesting_level,
                               recurse_info,
                               &info_count);
      if (ret == KERN_SUCCESS && ip >= addr && ip < (addr + size)) {
        
        
        ip_memory_d.start_of_memory_range =
          std::max(uintptr_t(addr),
                   uintptr_t(ip - (kIPMemorySize / 2)));
        uintptr_t end_of_range = 
          std::min(uintptr_t(ip + (kIPMemorySize / 2)),
                   uintptr_t(addr + size));
        ip_memory_d.memory.data_size =
            end_of_range -
            static_cast<uintptr_t>(ip_memory_d.start_of_memory_range);
        have_ip_memory = true;
        
        
        
        
        memory_blocks_.push_back(ip_memory_d);
      }
    }
  }

  
  unsigned memory_count = memory_blocks_.size();
  if (!list.AllocateObjectAndArray(memory_count,
                                   sizeof(MDMemoryDescriptor)))
    return false;

  memory_list_stream->stream_type = MD_MEMORY_LIST_STREAM;
  memory_list_stream->location = list.location();

  list.get()->number_of_memory_ranges = memory_count;

  unsigned int i;
  for (i = 0; i < memory_count; ++i) {
    list.CopyIndexAfterObject(i, &memory_blocks_[i],
                              sizeof(MDMemoryDescriptor));
  }

  if (have_ip_memory) {
    
    UntypedMDRVA ip_memory(&writer_);
    if (!ip_memory.Allocate(ip_memory_d.memory.data_size))
      return false;

    if (dynamic_images_) {
      
      vector<uint8_t> memory;
      if (ReadTaskMemory(crashing_task_,
                         ip_memory_d.start_of_memory_range,
                         ip_memory_d.memory.data_size,
                         memory) != KERN_SUCCESS) {
        return false;
      }

      ip_memory.Copy(&memory[0], ip_memory_d.memory.data_size);
    } else {
      
      ip_memory.Copy(
        reinterpret_cast<const void *>(ip_memory_d.start_of_memory_range),
        ip_memory_d.memory.data_size);
    }

    ip_memory_d.memory = ip_memory.location();
    
    list.CopyIndexAfterObject(i - 1, &ip_memory_d,
                              sizeof(MDMemoryDescriptor));
  }

  return true;
}

bool
MinidumpGenerator::WriteExceptionStream(MDRawDirectory *exception_stream) {
  TypedMDRVA<MDRawExceptionStream> exception(&writer_);

  if (!exception.Allocate())
    return false;

  exception_stream->stream_type = MD_EXCEPTION_STREAM;
  exception_stream->location = exception.location();
  MDRawExceptionStream *exception_ptr = exception.get();
  exception_ptr->thread_id = exception_thread_;

  
  
  exception_ptr->exception_record.exception_code = exception_type_;
  exception_ptr->exception_record.exception_flags = exception_code_;

  breakpad_thread_state_data_t state;
  mach_msg_type_number_t state_count
      = static_cast<mach_msg_type_number_t>(sizeof(state));

  if (!GetThreadState(exception_thread_, state, &state_count))
    return false;

  if (!WriteContext(state, &exception_ptr->thread_context))
    return false;

  if (exception_type_ == EXC_BAD_ACCESS)
    exception_ptr->exception_record.exception_address = exception_subcode_;
  else
    exception_ptr->exception_record.exception_address = CurrentPCForStack(state);

  return true;
}

bool MinidumpGenerator::WriteSystemInfoStream(
    MDRawDirectory *system_info_stream) {
  TypedMDRVA<MDRawSystemInfo> info(&writer_);

  if (!info.Allocate())
    return false;

  system_info_stream->stream_type = MD_SYSTEM_INFO_STREAM;
  system_info_stream->location = info.location();

  
  uint32_t number_of_processors;
  size_t len = sizeof(number_of_processors);
  sysctlbyname("hw.ncpu", &number_of_processors, &len, NULL, 0);
  MDRawSystemInfo *info_ptr = info.get();

  switch (cpu_type_) {
#ifdef HAS_ARM_SUPPORT
    case CPU_TYPE_ARM:
      info_ptr->processor_architecture = MD_CPU_ARCHITECTURE_ARM;
      break;
#endif
#ifdef HAS_PPC_SUPPORT
    case CPU_TYPE_POWERPC:
    case CPU_TYPE_POWERPC64:
      info_ptr->processor_architecture = MD_CPU_ARCHITECTURE_PPC;
      break;
#endif
#ifdef HAS_X86_SUPPORT
    case CPU_TYPE_I386:
    case CPU_TYPE_X86_64:
      if (cpu_type_ == CPU_TYPE_I386)
        info_ptr->processor_architecture = MD_CPU_ARCHITECTURE_X86;
      else
        info_ptr->processor_architecture = MD_CPU_ARCHITECTURE_AMD64;
#ifdef __i386__
      
      
#define cpuid(op,eax,ebx,ecx,edx)      \
  asm ("pushl %%ebx   \n\t"            \
       "cpuid         \n\t"            \
       "movl %%ebx,%1 \n\t"            \
       "popl %%ebx"                    \
       : "=a" (eax),                   \
         "=g" (ebx),                   \
         "=c" (ecx),                   \
         "=d" (edx)                    \
       : "0" (op))
#elif defined(__x86_64__)

#define cpuid(op,eax,ebx,ecx,edx)      \
  asm ("cpuid         \n\t"            \
       : "=a" (eax),                   \
         "=b" (ebx),                   \
         "=c" (ecx),                   \
         "=d" (edx)                    \
       : "0" (op))
#endif

#if defined(__i386__) || defined(__x86_64__)
      int unused, unused2;
      
      cpuid(0, unused, info_ptr->cpu.x86_cpu_info.vendor_id[0],
            info_ptr->cpu.x86_cpu_info.vendor_id[2],
            info_ptr->cpu.x86_cpu_info.vendor_id[1]);
      
      cpuid(1, info_ptr->cpu.x86_cpu_info.version_information, unused, unused2,
            info_ptr->cpu.x86_cpu_info.feature_information);

      
      info_ptr->processor_level =
        (info_ptr->cpu.x86_cpu_info.version_information & 0xF00) >> 8;
      
      info_ptr->processor_revision =
        (info_ptr->cpu.x86_cpu_info.version_information & 0xF) |
        ((info_ptr->cpu.x86_cpu_info.version_information & 0xF0) << 4);

      
      if (info_ptr->processor_level == 0xF ||
          info_ptr->processor_level == 0x6) {
        info_ptr->processor_revision |=
          ((info_ptr->cpu.x86_cpu_info.version_information & 0xF0000) >> 4);
      }

      
      if (info_ptr->processor_level == 0xF) {
        info_ptr->processor_level +=
          ((info_ptr->cpu.x86_cpu_info.version_information & 0xFF00000) >> 20);
      }

#endif  
      break;
#endif  
    default:
      info_ptr->processor_architecture = MD_CPU_ARCHITECTURE_UNKNOWN;
      break;
  }

  info_ptr->number_of_processors = static_cast<uint8_t>(number_of_processors);
#if TARGET_OS_IPHONE
  info_ptr->platform_id = MD_OS_IOS;
#else
  info_ptr->platform_id = MD_OS_MAC_OS_X;
#endif  

  MDLocationDescriptor build_string_loc;

  if (!writer_.WriteString(build_string_, 0,
                           &build_string_loc))
    return false;

  info_ptr->csd_version_rva = build_string_loc.rva;
  info_ptr->major_version = os_major_version_;
  info_ptr->minor_version = os_minor_version_;
  info_ptr->build_number = os_build_number_;

  return true;
}

bool MinidumpGenerator::WriteModuleStream(unsigned int index,
                                          MDRawModule *module) {
  if (dynamic_images_) {
    
    DynamicImage *image = dynamic_images_->GetImage(index);

    if (!image)
      return false;

    memset(module, 0, sizeof(MDRawModule));

    MDLocationDescriptor string_location;

    string name = image->GetFilePath();
    if (!writer_.WriteString(name.c_str(), 0, &string_location))
      return false;

    module->base_of_image = image->GetVMAddr() + image->GetVMAddrSlide();
    module->size_of_image = static_cast<uint32_t>(image->GetVMSize());
    module->module_name_rva = string_location.rva;

    
    
    
    if (index != static_cast<uint32_t>(FindExecutableModule())) {
      module->version_info.signature = MD_VSFIXEDFILEINFO_SIGNATURE;
      module->version_info.struct_version |= MD_VSFIXEDFILEINFO_VERSION;
      
      
      
      
      
      
      
      uint32_t modVersion = image->GetVersion();
      module->version_info.file_version_hi = 0;
      module->version_info.file_version_hi = modVersion >> 16;
      module->version_info.file_version_lo |= (modVersion & 0xff00)  << 8;
      module->version_info.file_version_lo |= (modVersion & 0xff);
    }

    if (!WriteCVRecord(module, image->GetCPUType(), name.c_str(), false)) {
      return false;
    }
  } else {
    
    const breakpad_mach_header *header;
    header = (breakpad_mach_header*)_dyld_get_image_header(index);
    if (!header)
      return false;

#ifdef __LP64__
    assert(header->magic == MH_MAGIC_64);

    if(header->magic != MH_MAGIC_64)
      return false;
#else
    assert(header->magic == MH_MAGIC);

    if(header->magic != MH_MAGIC)
      return false;
#endif

    int cpu_type = header->cputype;
    unsigned long slide = _dyld_get_image_vmaddr_slide(index);
    const char* name = _dyld_get_image_name(index);
    const struct load_command *cmd =
        reinterpret_cast<const struct load_command *>(header + 1);

    memset(module, 0, sizeof(MDRawModule));

    for (unsigned int i = 0; cmd && (i < header->ncmds); i++) {
      if (cmd->cmd == LC_SEGMENT_ARCH) {

        const breakpad_mach_segment_command *seg =
            reinterpret_cast<const breakpad_mach_segment_command *>(cmd);

        if (!strcmp(seg->segname, "__TEXT")) {
          MDLocationDescriptor string_location;

          if (!writer_.WriteString(name, 0, &string_location))
            return false;

          module->base_of_image = seg->vmaddr + slide;
          module->size_of_image = static_cast<uint32_t>(seg->vmsize);
          module->module_name_rva = string_location.rva;

          bool in_memory = false;
#if TARGET_OS_IPHONE
          in_memory = true;
#endif
          if (!WriteCVRecord(module, cpu_type, name, in_memory))
            return false;

          return true;
        }
      }

      cmd = reinterpret_cast<struct load_command*>((char *)cmd + cmd->cmdsize);
    }
  }

  return true;
}

int MinidumpGenerator::FindExecutableModule() {
  if (dynamic_images_) {
    int index = dynamic_images_->GetExecutableImageIndex();

    if (index >= 0) {
      return index;
    }
  } else {
    int image_count = _dyld_image_count();
    const struct mach_header *header;

    for (int index = 0; index < image_count; ++index) {
      header = _dyld_get_image_header(index);

      if (header->filetype == MH_EXECUTE)
        return index;
    }
  }

  
  return 0;
}

bool MinidumpGenerator::WriteCVRecord(MDRawModule *module, int cpu_type,
                                      const char *module_path, bool in_memory) {
  TypedMDRVA<MDCVInfoPDB70> cv(&writer_);

  
  const char *module_name = strrchr(module_path, '/');

  
  if (module_name)
    ++module_name;
  else
    module_name = "<Unknown>";

  size_t module_name_length = strlen(module_name);

  if (!cv.AllocateObjectAndArray(module_name_length + 1, sizeof(uint8_t)))
    return false;

  if (!cv.CopyIndexAfterObject(0, module_name, module_name_length))
    return false;

  module->cv_record = cv.location();
  MDCVInfoPDB70 *cv_ptr = cv.get();
  cv_ptr->cv_signature = MD_CVINFOPDB70_SIGNATURE;
  cv_ptr->age = 0;

  
  unsigned char identifier[16];
  bool result = false;
  if (in_memory) {
    MacFileUtilities::MachoID macho(module_path,
        reinterpret_cast<void *>(module->base_of_image),
        static_cast<size_t>(module->size_of_image));
    result = macho.UUIDCommand(cpu_type, CPU_SUBTYPE_MULTIPLE, identifier);
    if (!result)
      result = macho.MD5(cpu_type, CPU_SUBTYPE_MULTIPLE, identifier);
  }

  if (!result) {
     FileID file_id(module_path);
     result = file_id.MachoIdentifier(cpu_type, CPU_SUBTYPE_MULTIPLE,
                                      identifier);
  }

  if (result) {
    cv_ptr->signature.data1 =
        static_cast<uint32_t>(identifier[0]) << 24 |
        static_cast<uint32_t>(identifier[1]) << 16 |
        static_cast<uint32_t>(identifier[2]) << 8 |
        static_cast<uint32_t>(identifier[3]);
    cv_ptr->signature.data2 =
        static_cast<uint16_t>(identifier[4] << 8) | identifier[5];
    cv_ptr->signature.data3 =
        static_cast<uint16_t>(identifier[6] << 8) | identifier[7];
    cv_ptr->signature.data4[0] = identifier[8];
    cv_ptr->signature.data4[1] = identifier[9];
    cv_ptr->signature.data4[2] = identifier[10];
    cv_ptr->signature.data4[3] = identifier[11];
    cv_ptr->signature.data4[4] = identifier[12];
    cv_ptr->signature.data4[5] = identifier[13];
    cv_ptr->signature.data4[6] = identifier[14];
    cv_ptr->signature.data4[7] = identifier[15];
  }

  return true;
}

bool MinidumpGenerator::WriteModuleListStream(
    MDRawDirectory *module_list_stream) {
  TypedMDRVA<MDRawModuleList> list(&writer_);

  size_t image_count = dynamic_images_ ?
      static_cast<size_t>(dynamic_images_->GetImageCount()) :
      _dyld_image_count();

  if (!list.AllocateObjectAndArray(image_count, MD_MODULE_SIZE))
    return false;

  module_list_stream->stream_type = MD_MODULE_LIST_STREAM;
  module_list_stream->location = list.location();
  list.get()->number_of_modules = image_count;

  
  MDRawModule module;
  size_t executableIndex = FindExecutableModule();

  if (!WriteModuleStream(executableIndex, &module)) {
    return false;
  }

  list.CopyIndexAfterObject(0, &module, MD_MODULE_SIZE);
  int destinationIndex = 1;  

  for (size_t i = 0; i < image_count; ++i) {
    if (i != executableIndex) {
      if (!WriteModuleStream(i, &module)) {
        return false;
      }

      list.CopyIndexAfterObject(destinationIndex++, &module, MD_MODULE_SIZE);
    }
  }

  return true;
}

bool MinidumpGenerator::WriteMiscInfoStream(MDRawDirectory *misc_info_stream) {
  TypedMDRVA<MDRawMiscInfo> info(&writer_);

  if (!info.Allocate())
    return false;

  misc_info_stream->stream_type = MD_MISC_INFO_STREAM;
  misc_info_stream->location = info.location();

  MDRawMiscInfo *info_ptr = info.get();
  info_ptr->size_of_info = static_cast<uint32_t>(sizeof(MDRawMiscInfo));
  info_ptr->flags1 = MD_MISCINFO_FLAGS1_PROCESS_ID |
    MD_MISCINFO_FLAGS1_PROCESS_TIMES |
    MD_MISCINFO_FLAGS1_PROCESSOR_POWER_INFO;

  
  info_ptr->process_id = getpid();

  
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) != -1) {
    
    info_ptr->process_user_time =
        static_cast<uint32_t>(usage.ru_utime.tv_sec);
    info_ptr->process_kernel_time =
        static_cast<uint32_t>(usage.ru_stime.tv_sec);
  }
  int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID,
                 static_cast<int>(info_ptr->process_id) };
  uint mibsize = static_cast<uint>(sizeof(mib) / sizeof(mib[0]));
  struct kinfo_proc proc;
  size_t size = sizeof(proc);
  if (sysctl(mib, mibsize, &proc, &size, NULL, 0) == 0) {
    info_ptr->process_create_time =
        static_cast<uint32_t>(proc.kp_proc.p_starttime.tv_sec);
  }

  
  uint64_t speed;
  const uint64_t kOneMillion = 1000 * 1000;
  size = sizeof(speed);
  sysctlbyname("hw.cpufrequency_max", &speed, &size, NULL, 0);
  info_ptr->processor_max_mhz = static_cast<uint32_t>(speed / kOneMillion);
  info_ptr->processor_mhz_limit = static_cast<uint32_t>(speed / kOneMillion);
  size = sizeof(speed);
  sysctlbyname("hw.cpufrequency", &speed, &size, NULL, 0);
  info_ptr->processor_current_mhz = static_cast<uint32_t>(speed / kOneMillion);

  return true;
}

bool MinidumpGenerator::WriteBreakpadInfoStream(
    MDRawDirectory *breakpad_info_stream) {
  TypedMDRVA<MDRawBreakpadInfo> info(&writer_);

  if (!info.Allocate())
    return false;

  breakpad_info_stream->stream_type = MD_BREAKPAD_INFO_STREAM;
  breakpad_info_stream->location = info.location();
  MDRawBreakpadInfo *info_ptr = info.get();

  if (exception_thread_ && exception_type_) {
    info_ptr->validity = MD_BREAKPAD_INFO_VALID_DUMP_THREAD_ID |
                         MD_BREAKPAD_INFO_VALID_REQUESTING_THREAD_ID;
    info_ptr->dump_thread_id = handler_thread_;
    info_ptr->requesting_thread_id = exception_thread_;
  } else {
    info_ptr->validity = MD_BREAKPAD_INFO_VALID_DUMP_THREAD_ID;
    info_ptr->dump_thread_id = handler_thread_;
    info_ptr->requesting_thread_id = 0;
  }

  return true;
}

}  
