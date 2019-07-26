



#pragma once

#include <string>

#include "CC_Common.h"
#include "ECC_Types.h"

namespace CSF
{
	DECLARE_PTR_VECTOR(PhoneDetails);
	class ECC_API PhoneDetails
	{
	public:
		virtual ~PhoneDetails() {}
		


		virtual std::string getName() const = 0;
		virtual std::string getDescription() const = 0;

		




		virtual int getModel() const = 0;
		virtual std::string getModelDescription() const = 0;

		virtual bool isSoftPhone() = 0;

		



		virtual std::vector<std::string> getLineDNs() const = 0;

		


		virtual ServiceStateType::ServiceState getServiceState() const = 0;

		


		virtual std::string getConfig() const = 0;

	protected:
		PhoneDetails() {}

	private:
		PhoneDetails(const PhoneDetails&);
		PhoneDetails& operator=(const PhoneDetails&);
	};
};
