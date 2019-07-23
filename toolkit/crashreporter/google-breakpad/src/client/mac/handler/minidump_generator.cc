




























#include <cstdio>

#include <mach/host_info.h>
#include <mach/mach_vm.h>
#include <mach/vm_statistics.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <sys/sysctl.h>
#include <sys/resource.h>
#include <mach/mach_vm.h>

#include <CoreFoundation/CoreFoundation.h>

#include "client/mac/handler/minidump_generator.h"
#include "client/minidump_file_writer-inl.h"
#include "common/mac/file_id.h"
#include "common/mac/string_utilities.h"

using MacStringUtils::ConvertToString;
using MacStringUtils::IntegerValueAtIndex;

namespace google_breakpad {


MinidumpGenerator::MinidumpGenerator()
    : exception_type_(0),
      exception_code_(0),
      exception_subcode_(0),
      exception_thread_(0),
      crashing_task_(mach_task_self()),
      handler_thread_(mach_thread_self()),
      dynamic_images_(NULL) {
  GatherSystemInformation();
}



MinidumpGenerator::MinidumpGenerator(mach_port_t crashing_task,
                                     mach_port_t handler_thread)
    : exception_type_(0),
      exception_code_(0),
      exception_subcode_(0),
      exception_thread_(0),
      crashing_task_(crashing_task),
      handler_thread_(handler_thread) {
  if (crashing_task != mach_task_self()) {
    dynamic_images_ = new DynamicImages(crashing_task_);
  } else {
    dynamic_images_ = NULL;
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

  if (!data)
    return;

  CFDictionaryRef list = static_cast<CFDictionaryRef>
    (CFPropertyListCreateFromXMLData(NULL, data, kCFPropertyListImmutable,
                                     NULL));
  if (!list)
    return;

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

    int writer_count = sizeof(writers) / sizeof(writers[0]);

    
    
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
                           region_info,
                           &info_count);

  if (start_addr < stack_region_base) {
    
    
    return 0;
  }


  if ((stack_region_base + stack_region_size) == TOP_OF_THREAD0_STACK) {
    
    
    
    
    

    
    

    
    
    
    
    stack_region_size += 0x1000;
  }

  return result == KERN_SUCCESS ?
    stack_region_base + stack_region_size - start_addr : 0;
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

      kern_return_t kr;

      void *stack_memory = ReadTaskMemory(crashing_task_,
                                          (void*)start_addr,
                                          size,
                                          &kr);

      if (stack_memory == NULL) {
        return false;
      }

      result = memory.Copy(stack_memory, size);
      free(stack_memory);
    } else {
      result = memory.Copy(reinterpret_cast<const void *>(start_addr), size);
    }
  }

  stack_location->start_of_memory_range = start_addr;
  stack_location->memory = memory.location();

  return result;
}

#if TARGET_CPU_PPC || TARGET_CPU_PPC64
bool MinidumpGenerator::WriteStack(breakpad_thread_state_data_t state,
                                   MDMemoryDescriptor *stack_location) {
  breakpad_thread_state_t *machine_state =
    reinterpret_cast<breakpad_thread_state_t *>(state);
#if TARGET_CPU_PPC
  mach_vm_address_t start_addr = machine_state->r1;
#else
  mach_vm_address_t start_addr = machine_state->__r1;
#endif
  return WriteStackFromStartAddress(start_addr, stack_location);
}

u_int64_t
MinidumpGenerator::CurrentPCForStack(breakpad_thread_state_data_t state) {
  breakpad_thread_state_t *machine_state =
    reinterpret_cast<breakpad_thread_state_t *>(state);

#if TARGET_CPU_PPC
  return machine_state->srr0;
#else
  return machine_state->__srr0;
#endif
}

bool MinidumpGenerator::WriteContext(breakpad_thread_state_data_t state,
                                     MDLocationDescriptor *register_location) {
  TypedMDRVA<MinidumpContext> context(&writer_);
  breakpad_thread_state_t *machine_state =
    reinterpret_cast<breakpad_thread_state_t *>(state);

  if (!context.Allocate())
    return false;

  *register_location = context.location();
  MinidumpContext *context_ptr = context.get();
  context_ptr->context_flags = MD_CONTEXT_PPC_BASE;

#if TARGET_CPU_PPC64
#define AddReg(a) context_ptr->a = machine_state->__ ## a
#define AddGPR(a) context_ptr->gpr[a] = machine_state->__r ## a
#else
#define AddReg(a) context_ptr->a = machine_state->a
#define AddGPR(a) context_ptr->gpr[a] = machine_state->r ## a
#endif
 
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

#if TARGET_CPU_PPC
  
  AddReg(mq);
#endif


  return true;
}

#elif TARGET_CPU_X86 || TARGET_CPU_X86_64

bool MinidumpGenerator::WriteStack(breakpad_thread_state_data_t state,
                                   MDMemoryDescriptor *stack_location) {
  breakpad_thread_state_t *machine_state =
    reinterpret_cast<breakpad_thread_state_t *>(state);

#if TARGET_CPU_X86_64
  mach_vm_address_t start_addr = REGISTER_FROM_THREADSTATE(machine_state, rsp);
#else
  mach_vm_address_t start_addr = REGISTER_FROM_THREADSTATE(machine_state, esp);
#endif
  return WriteStackFromStartAddress(start_addr, stack_location);
}

u_int64_t
MinidumpGenerator::CurrentPCForStack(breakpad_thread_state_data_t state) {
  breakpad_thread_state_t *machine_state =
    reinterpret_cast<breakpad_thread_state_t *>(state);

#if TARGET_CPU_X86_64
  return REGISTER_FROM_THREADSTATE(machine_state, rip);
#else
  return REGISTER_FROM_THREADSTATE(machine_state, eip);
#endif
}

bool MinidumpGenerator::WriteContext(breakpad_thread_state_data_t state,
                                     MDLocationDescriptor *register_location) {
  TypedMDRVA<MinidumpContext> context(&writer_);
  breakpad_thread_state_t *machine_state =
    reinterpret_cast<breakpad_thread_state_t *>(state);

  if (!context.Allocate())
    return false;

  *register_location = context.location();
  MinidumpContext *context_ptr = context.get();

#define AddReg(a) context_ptr->a = REGISTER_FROM_THREADSTATE(machine_state, a)
#if TARGET_CPU_X86
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
#else
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
  
  
  
  
  context_ptr->eflags = machine_state->__rflags;
  AddReg(cs);
  AddReg(fs);
  AddReg(gs);
#endif
#undef AddReg(a)

  return true;
}
#endif

bool MinidumpGenerator::WriteThreadStream(mach_port_t thread_id,
                                          MDRawThread *thread) {
  breakpad_thread_state_data_t state;
  mach_msg_type_number_t state_count = sizeof(state);

  if (thread_get_state(thread_id, BREAKPAD_MACHINE_THREAD_STATE,
                       state, &state_count) ==
      KERN_SUCCESS) {
    if (!WriteStack(state, &thread->stack))
      return false;

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

  
  non_generator_thread_count = thread_count - 1;
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
  mach_msg_type_number_t stateCount = sizeof(state);

  if (thread_get_state(exception_thread_,
                       BREAKPAD_MACHINE_THREAD_STATE,
                       state,
                       &stateCount) != KERN_SUCCESS)
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

  
  uint32_t cpu_type;
  size_t len = sizeof(cpu_type);
  sysctlbyname("hw.cputype", &cpu_type, &len, NULL, 0);
  uint32_t number_of_processors;
  len = sizeof(number_of_processors);
  sysctlbyname("hw.ncpu", &number_of_processors, &len, NULL, 0);
  MDRawSystemInfo *info_ptr = info.get();

  switch (cpu_type) {
    case CPU_TYPE_POWERPC:
      info_ptr->processor_architecture = MD_CPU_ARCHITECTURE_PPC;
      break;
    case CPU_TYPE_I386:
      info_ptr->processor_architecture = MD_CPU_ARCHITECTURE_X86;
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
#endif 
      break;
    default:
      info_ptr->processor_architecture = MD_CPU_ARCHITECTURE_UNKNOWN;
      break;
  }

  info_ptr->number_of_processors = number_of_processors;
  info_ptr->platform_id = MD_OS_MAC_OS_X;

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

    const breakpad_mach_header *header = image->GetMachHeader();

    if (!header)
      return false;

    int cpu_type = header->cputype;

    memset(module, 0, sizeof(MDRawModule));

    MDLocationDescriptor string_location;

    const char* name = image->GetFilePath();
    if (!writer_.WriteString(name, 0, &string_location))
      return false;

    module->base_of_image = image->GetVMAddr() + image->GetVMAddrSlide();
    module->size_of_image = image->GetVMSize();
    module->module_name_rva = string_location.rva;

    
    
    
    if (index != (uint32_t)FindExecutableModule()) {
      module->version_info.signature = MD_VSFIXEDFILEINFO_SIGNATURE;
      module->version_info.struct_version |= MD_VSFIXEDFILEINFO_VERSION;
      
      
      
      
      
      
      
      uint32_t modVersion = image->GetVersion();
      module->version_info.file_version_hi = 0;
      module->version_info.file_version_hi = modVersion >> 16;
      module->version_info.file_version_lo |= (modVersion & 0xff00)  << 8;
      module->version_info.file_version_lo |= (modVersion & 0xff);
    }

    if (!WriteCVRecord(module, cpu_type, name)) {
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
      if (cmd->cmd == LC_SEGMENT) {

        const breakpad_mach_segment_command *seg =
          reinterpret_cast<const breakpad_mach_segment_command *>(cmd);

        if (!strcmp(seg->segname, "__TEXT")) {
          MDLocationDescriptor string_location;

          if (!writer_.WriteString(name, 0, &string_location))
            return false;

          module->base_of_image = seg->vmaddr + slide;
          module->size_of_image = seg->vmsize;
          module->module_name_rva = string_location.rva;

          if (!WriteCVRecord(module, cpu_type, name))
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
                                      const char *module_path) {
  TypedMDRVA<MDCVInfoPDB70> cv(&writer_);

  
  const char *module_name = strrchr(module_path, '/');

  
  if (module_name)
    ++module_name;
  else
    module_name = "<Unknown>";

  size_t module_name_length = strlen(module_name);

  if (!cv.AllocateObjectAndArray(module_name_length + 1, sizeof(u_int8_t)))
    return false;

  if (!cv.CopyIndexAfterObject(0, module_name, module_name_length))
    return false;

  module->cv_record = cv.location();
  MDCVInfoPDB70 *cv_ptr = cv.get();
  cv_ptr->cv_signature = MD_CVINFOPDB70_SIGNATURE;
  cv_ptr->age = 0;

  
  FileID file_id(module_path);
  unsigned char identifier[16];

  if (file_id.MachoIdentifier(cpu_type, identifier)) {
    cv_ptr->signature.data1 = (uint32_t)identifier[0] << 24 |
      (uint32_t)identifier[1] << 16 | (uint32_t)identifier[2] << 8 |
      (uint32_t)identifier[3];
    cv_ptr->signature.data2 = (uint32_t)identifier[4] << 8 | identifier[5];
    cv_ptr->signature.data3 = (uint32_t)identifier[6] << 8 | identifier[7];
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

  int image_count = dynamic_images_ ?
    dynamic_images_->GetImageCount() : _dyld_image_count();

  if (!list.AllocateObjectAndArray(image_count, MD_MODULE_SIZE))
    return false;

  module_list_stream->stream_type = MD_MODULE_LIST_STREAM;
  module_list_stream->location = list.location();
  list.get()->number_of_modules = image_count;

  
  MDRawModule module;
  int executableIndex = FindExecutableModule();

  if (!WriteModuleStream(executableIndex, &module)) {
    return false;
  }

  list.CopyIndexAfterObject(0, &module, MD_MODULE_SIZE);
  int destinationIndex = 1;  

  for (int i = 0; i < image_count; ++i) {
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
  info_ptr->size_of_info = sizeof(MDRawMiscInfo);
  info_ptr->flags1 = MD_MISCINFO_FLAGS1_PROCESS_ID |
    MD_MISCINFO_FLAGS1_PROCESS_TIMES |
    MD_MISCINFO_FLAGS1_PROCESSOR_POWER_INFO;

  
  info_ptr->process_id = getpid();

  
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) != -1) {
    
    info_ptr->process_user_time = usage.ru_utime.tv_sec;
    info_ptr->process_kernel_time = usage.ru_stime.tv_sec;
  }
  int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, info_ptr->process_id };
  size_t size;
  if (!sysctl(mib, sizeof(mib) / sizeof(mib[0]), NULL, &size, NULL, 0)) {
    mach_vm_address_t addr;
    if (mach_vm_allocate(mach_task_self(),
                         &addr,
                         size,
                         true) == KERN_SUCCESS) {
      struct kinfo_proc *proc = (struct kinfo_proc *)addr;
      if (!sysctl(mib, sizeof(mib) / sizeof(mib[0]), proc, &size, NULL, 0))
        info_ptr->process_create_time = proc->kp_proc.p_starttime.tv_sec;
      mach_vm_deallocate(mach_task_self(), addr, size);
    }
  }

  
  uint64_t speed;
  size = sizeof(speed);
  sysctlbyname("hw.cpufrequency_max", &speed, &size, NULL, 0);
  info_ptr->processor_max_mhz = speed / (1000 * 1000);
  info_ptr->processor_mhz_limit = speed / (1000 * 1000);
  size = sizeof(speed);
  sysctlbyname("hw.cpufrequency", &speed, &size, NULL, 0);
  info_ptr->processor_current_mhz = speed / (1000 * 1000);

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
