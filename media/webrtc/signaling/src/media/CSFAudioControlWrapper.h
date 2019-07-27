



#pragma once

#include "mozilla/RefPtr.h"
#include "CC_Common.h"
#include "CSFAudioControl.h"

namespace CSF
{
        DECLARE_NS_PTR(AudioControlWrapper)
	class ECC_API AudioControlWrapper : public AudioControl
	{
	public:
		

		explicit AudioControlWrapper(AudioControl * audioControl){_realAudioControl = audioControl;};
		virtual std::vector<std::string> getRecordingDevices();
		virtual std::vector<std::string> getPlayoutDevices();

		virtual std::string getRecordingDevice();
		virtual std::string getPlayoutDevice();

		virtual bool setRecordingDevice( const std::string& name );
		virtual bool setPlayoutDevice( const std::string& name );

        virtual bool setDefaultVolume( int volume );
        virtual int getDefaultVolume();

        virtual bool setRingerVolume( int volume );
        virtual int getRingerVolume();

		virtual void setAudioControl(AudioControl * audioControl){_realAudioControl = audioControl;};

	private:
		virtual ~AudioControlWrapper();

		mozilla::RefPtr<AudioControl> _realAudioControl;
	};
};
