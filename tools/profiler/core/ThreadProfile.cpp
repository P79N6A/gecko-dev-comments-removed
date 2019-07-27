





ThreadProfile::ThreadProfile(ThreadInfo* aInfo, ProfileBuffer* aBuffer)
  : mThreadInfo(aInfo)
  , mBuffer(aBuffer)
  , mPseudoStack(aInfo->Stack())
  , mMutex(OS::CreateMutex("ThreadProfile::mMutex"))
  , mThreadId(int(aInfo->ThreadId()))
  , mIsMainThread(aInfo->IsMainThread())
  , mPlatformData(aInfo->GetPlatformData())
  , mStackTop(aInfo->StackTop())
#ifndef SPS_STANDALONE
  , mRespInfo(this)
#endif
#ifdef XP_LINUX
  , mRssMemory(0)
  , mUssMemory(0)
#endif
{
  MOZ_COUNT_CTOR(ThreadProfile);
  MOZ_ASSERT(aBuffer);

  
  MOZ_ASSERT(aInfo->ThreadId() >= 0, "native thread ID is < 0");
  MOZ_ASSERT(aInfo->ThreadId() <= INT32_MAX, "native thread ID is > INT32_MAX");
}

ThreadProfile::~ThreadProfile()
{
  MOZ_COUNT_DTOR(ThreadProfile);
}

void ThreadProfile::addTag(const ProfileEntry& aTag)
{
  mBuffer->addTag(aTag);
}

void ThreadProfile::addStoredMarker(ProfilerMarker *aStoredMarker) {
  mBuffer->addStoredMarker(aStoredMarker);
}

void ThreadProfile::StreamJSON(SpliceableJSONWriter& aWriter, double aSinceTime)
{
  
  if (!mUniqueStacks.isSome()) {
#ifndef SPS_STANDALONE
    mUniqueStacks.emplace(mPseudoStack->mRuntime);
#else
    mUniqueStacks.emplace(nullptr);
#endif
  }

  aWriter.Start(SpliceableJSONWriter::SingleLineStyle);
  {
    StreamSamplesAndMarkers(aWriter, aSinceTime, *mUniqueStacks);

    aWriter.StartObjectProperty("stackTable");
    {
      {
        JSONSchemaWriter schema(aWriter);
        schema.WriteField("prefix");
        schema.WriteField("frame");
      }

      aWriter.StartArrayProperty("data");
      {
        mUniqueStacks->SpliceStackTableElements(aWriter);
      }
      aWriter.EndArray();
    }
    aWriter.EndObject();

    aWriter.StartObjectProperty("frameTable");
    {
      {
        JSONSchemaWriter schema(aWriter);
        schema.WriteField("location");
        schema.WriteField("implementation");
        schema.WriteField("optimizations");
        schema.WriteField("line");
        schema.WriteField("category");
      }

      aWriter.StartArrayProperty("data");
      {
        mUniqueStacks->SpliceFrameTableElements(aWriter);
      }
      aWriter.EndArray();
    }
    aWriter.EndObject();

    aWriter.StartArrayProperty("stringTable");
    {
      mUniqueStacks->mUniqueStrings.SpliceStringTableElements(aWriter);
    }
    aWriter.EndArray();
  }
  aWriter.End();

  mUniqueStacks.reset();
}

void ThreadProfile::StreamSamplesAndMarkers(SpliceableJSONWriter& aWriter, double aSinceTime,
                                            UniqueStacks& aUniqueStacks)
{
#ifndef SPS_STANDALONE
  
  if (XRE_GetProcessType() == GeckoProcessType_Plugin) {
    
    aWriter.StringProperty("name", "Plugin");
  } else if (XRE_GetProcessType() == GeckoProcessType_Content) {
    
    
    aWriter.StringProperty("name", "Content");
  } else {
    aWriter.StringProperty("name", Name());
  }
#else
  aWriter.StringProperty("name", Name());
#endif

  aWriter.IntProperty("tid", static_cast<int>(mThreadId));

  aWriter.StartObjectProperty("samples");
  {
    {
      JSONSchemaWriter schema(aWriter);
      schema.WriteField("stack");
      schema.WriteField("time");
      schema.WriteField("responsiveness");
      schema.WriteField("rss");
      schema.WriteField("uss");
      schema.WriteField("frameNumber");
      schema.WriteField("power");
    }

    aWriter.StartArrayProperty("data");
    {
      if (mSavedStreamedSamples) {
        
        
        
        MOZ_ASSERT(aSinceTime == 0);
        aWriter.Splice(mSavedStreamedSamples.get());
        mSavedStreamedSamples.reset();
      }
      mBuffer->StreamSamplesToJSON(aWriter, mThreadId, aSinceTime,
#ifndef SPS_STANDALONE
                                   mPseudoStack->mRuntime,
#else
                                   nullptr,
#endif
                                   aUniqueStacks);
    }
    aWriter.EndArray();
  }
  aWriter.EndObject();

  aWriter.StartObjectProperty("markers");
  {
    {
      JSONSchemaWriter schema(aWriter);
      schema.WriteField("name");
      schema.WriteField("time");
      schema.WriteField("data");
    }

    aWriter.StartArrayProperty("data");
    {
      if (mSavedStreamedMarkers) {
        MOZ_ASSERT(aSinceTime == 0);
        aWriter.Splice(mSavedStreamedMarkers.get());
        mSavedStreamedMarkers.reset();
      }
      mBuffer->StreamMarkersToJSON(aWriter, mThreadId, aSinceTime, aUniqueStacks);
    }
    aWriter.EndArray();
  }
  aWriter.EndObject();
}

void ThreadProfile::FlushSamplesAndMarkers()
{
  
  
  MOZ_ASSERT(mPseudoStack->mRuntime);

  
  
  
  
  
  
  
#ifndef SPS_STANDALONE
  mUniqueStacks.emplace(mPseudoStack->mRuntime);
#else
  mUniqueStacks.emplace(nullptr);
#endif

  {
    SpliceableChunkedJSONWriter b;
    b.StartBareList();
    {
      mBuffer->StreamSamplesToJSON(b, mThreadId,  0,
#ifndef SPS_STANDALONE
                                   mPseudoStack->mRuntime,
#else
                                   nullptr,
#endif
                                   *mUniqueStacks);
    }
    b.EndBareList();
    mSavedStreamedSamples = b.WriteFunc()->CopyData();
  }

  {
    SpliceableChunkedJSONWriter b;
    b.StartBareList();
    {
      mBuffer->StreamMarkersToJSON(b, mThreadId,  0, *mUniqueStacks);
    }
    b.EndBareList();
    mSavedStreamedMarkers = b.WriteFunc()->CopyData();
  }

  
  
  mBuffer->reset();
}

PseudoStack* ThreadProfile::GetPseudoStack()
{
  return mPseudoStack;
}

void ThreadProfile::BeginUnwind()
{
  mMutex->Lock();
}

void ThreadProfile::EndUnwind()
{
  mMutex->Unlock();
}

::Mutex& ThreadProfile::GetMutex()
{
  return *mMutex.get();
}

void ThreadProfile::DuplicateLastSample()
{
  mBuffer->DuplicateLastSample(mThreadId);
}

