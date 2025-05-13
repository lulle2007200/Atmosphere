#include <exosphere.hpp>

namespace ams::nxboot {
	enum BootStorage : u32 {
		BootDrive_Min      = 0,
		BootDrive_Boot1Off = 0,
		BootDrive_Boot1,
		BootDrive_System,
		BootDrive_SD,
		BootDrive_Max,
		BootDrive_Invalid = BootDrive_Max,
	};


	Result MountBootStorage();
	void FinalizeBootStorage();
	void UnmountBootStorage();
	BootStorage GetBootStorage();
}