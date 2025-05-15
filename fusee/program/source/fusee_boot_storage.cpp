#include "fusee_boot_storage.hpp"
#include "fs/fusee_fs_api.hpp"
#include "fusee_mmc.hpp"
#include "fusee_sd_card.hpp"
#include <exosphere.hpp>

namespace ams::nxboot {
	namespace {
		constinit BootStorage g_boot_storage = BootDrive_Invalid;

		Result IsBootStorageValid() {
			fs::DirectoryEntryType entry_type;
			bool is_archive;
			Result r;
			if(R_SUCCEEDED(r = fs::GetEntryType(std::addressof(entry_type), std::addressof(is_archive), ".no_boot_storage"))) {
				if(entry_type == fs::DirectoryEntryType_File) {
					R_THROW(fs::ResultInvalidMountName());
				}
			}
			R_SUCCEED();
		}

		Result BootStorageInitializeAndMount(BootStorage storage) {
			AMS_ASSERT(g_boot_storage == BootDrive_Invalid);

			switch(storage) {
			case BootDrive_Boot1Off:
				{
					R_TRY(InitializeMmc());
					ON_RESULT_FAILURE { FinalizeMmc(); };
					R_UNLESS(fs::MountBoot1Off(), fs::ResultInvalidMountName());
					ON_RESULT_FAILURE_2 { fs::UnmountBoot1Off(); };
					R_TRY(fs::ChangeDrive("boot1_off:"));
					R_TRY(IsBootStorageValid());
					g_boot_storage = storage;
					R_SUCCEED();
				} break;
			case BootDrive_Boot1:
				{
					R_TRY(InitializeMmc());
					ON_RESULT_FAILURE { FinalizeMmc(); };
					R_UNLESS(fs::MountBoot1(), fs::ResultInvalidMountName());
					ON_RESULT_FAILURE_2 { fs::UnmountBoot1(); };
					R_TRY(fs::ChangeDrive("boot1:"));
					R_TRY(IsBootStorageValid());
					g_boot_storage = storage;
					R_SUCCEED();
				} break;
			case BootDrive_System:
				{
					R_TRY(InitializeMmc());
					ON_RESULT_FAILURE { FinalizeMmc(); };
					R_UNLESS(fs::MountSys(), fs::ResultInvalidMountName());
					ON_RESULT_FAILURE_2 { fs::UnmountSys(); };
					R_TRY(fs::ChangeDrive("sys:"));
					R_TRY(IsBootStorageValid());
					g_boot_storage = storage;
					R_SUCCEED();
				} break;
			case BootDrive_SD:
				{
					R_TRY(InitializeSdCard());
					ON_RESULT_FAILURE { FinalizeSdCard(); };
					R_UNLESS(fs::MountSdCard(), fs::ResultInvalidMountName());
					ON_RESULT_FAILURE_2 { fs::UnmountSdCard(); };
					R_TRY(fs::ChangeDrive("sdmc:"));
					R_TRY(IsBootStorageValid());
					g_boot_storage = storage;
					R_SUCCEED();
				} break;
			AMS_UNREACHABLE_DEFAULT_CASE();
			}

			R_THROW(fs::ResultInvalidMountName());
		}
	}

	Result MountBootStorage() {
		for(uint32_t i = BootDrive_Min; i < BootDrive_Max; i++) {
			R_SUCCEED_IF(R_SUCCEEDED(BootStorageInitializeAndMount(static_cast<BootStorage>(i))));
		}
		R_THROW(fs::ResultInvalidMountName());
	}

	void FinalizeBootStorage() {
		AMS_ASSERT(g_boot_storage != BootDrive_Invalid);

		switch(g_boot_storage) {
			case BootDrive_Boot1Off:
				fs::UnmountBoot1Off();
				FinalizeMmc();
				break;
			case BootDrive_Boot1:
				fs::UnmountBoot1();
				FinalizeMmc();
				break;
			case BootDrive_System:
				fs::UnmountSys();
				FinalizeMmc();
				break;
			case BootDrive_SD:
				fs::UnmountSdCard();
				FinalizeSdCard();
				break;
			AMS_UNREACHABLE_DEFAULT_CASE();
			}
	}

	void UnmountBootStorage() {
		AMS_ASSERT(g_boot_storage != BootDrive_Invalid);

		switch(g_boot_storage) {
			case BootDrive_Boot1Off:
				fs::UnmountBoot1Off();
				break;
			case BootDrive_Boot1:
				fs::UnmountBoot1();
				break;
			case BootDrive_System:
				fs::UnmountSys();
				break;
			case BootDrive_SD:
				fs::UnmountSdCard();
				break;
			AMS_UNREACHABLE_DEFAULT_CASE();
			}
	}

	BootStorage GetBootStorage(){
		return g_boot_storage;
	}

}