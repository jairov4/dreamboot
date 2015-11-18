/*
 * UEFI:SIMPLE - UEFI development made easy
 * Copyright © 2014-2015 Pete Batard <pete@akeo.ie> - Public Domain
 * See COPYING for the full licensing terms.
 */
#include <efi.h>
#include <efilib.h>

EFI_STATUS FindImageFile(CHAR16* diskImageFileName, EFI_HANDLE* handle, SIMPLE_READ_FILE* readHandle)
{
	EFI_HANDLE* handleArray;
	UINTN nbHandles;
	
	// Find file systems
	EFI_STATUS err = BS->LocateHandleBuffer(ByProtocol, &FileSystemProtocol, NULL, &nbHandles, &handleArray);
	if (err != EFI_SUCCESS)
	{
		return err;
	}

	Print(L"Located %d handles implementing FileSystemProtocol\n", nbHandles);

	for (UINTN i = 0; i < nbHandles; i++)
	{
		Print(L"Trying handle %d\n", i);

		EFI_DEVICE_PATH* path = FileDevicePath(handleArray[i], diskImageFileName);
		if (path == NULL)
		{
			continue;
		}

		err = OpenSimpleReadFile(TRUE, NULL, 0, &path, handle, readHandle);
		if(err == EFI_SUCCESS)
		{
			Print(L"File found and now is open\n");
			FreePool(handleArray);
			return EFI_SUCCESS;
		}
	}

	Print(L"File NOT found\n");
	FreePool(handleArray);

	return EFI_NOT_FOUND;
}


EFI_STATUS FindTargetDisk2(EFI_BLOCK_IO** block, EFI_DISK_IO** disk, UINT32* mediaId)
{	
	EFI_HANDLE* handleArray;
	UINTN nbHandles;

	// Find Disk IO interfaces
	EFI_STATUS err = BS->LocateHandleBuffer(ByProtocol, &DiskIoProtocol, NULL, &nbHandles, &handleArray);
	if (err != EFI_SUCCESS)
	{
		return EFI_NOT_FOUND;
	}

	for (UINTN i = 0; i < nbHandles; i++)
	{
		// Open the Device IO protocol
		err = BS->HandleProtocol(handleArray[i], &DiskIoProtocol, (void **)disk);
		if (err != EFI_SUCCESS)
		{
			continue;
		}

		err = BS->HandleProtocol(handleArray[i], &BlockIoProtocol, (void**)block);
		if (err != EFI_SUCCESS)
		{
			continue;
		}

		*mediaId = (*block)->Media->MediaId;
		Print(L"Detected MediaID: %d\n", *mediaId);
		Print(L"Detected Size: %d\n", (*block)->Media->LastBlock * (*block)->Media->BlockSize);
		Print(L"Detected IsLogical: %d\n", (*block)->Media->LogicalPartition);

		//FreePool(handleArray);
		//return EFI_SUCCESS;
	}

	FreePool(handleArray);
	return EFI_NOT_FOUND;
}

EFI_STATUS FindTargetDisk(CHAR16* path, EFI_BLOCK_IO** block, EFI_DISK_IO** disk)
{
	EFI_STATUS err;	
	EFI_DEVICE_PATH* devicePath = FileDevicePath(NULL, path);
	if (devicePath == NULL)
	{
		return EFI_NOT_FOUND;
	}
	Print(L"Device path built\n");

	EFI_HANDLE deviceHandle;
	err = LibDevicePathToInterface(&DiskIoProtocol, devicePath, (void**)disk);
	if (err != EFI_SUCCESS)
	{
		return EFI_NOT_FOUND;
	}
	Print(L"Device implementing DiskIoProtocol interface retrieved\n");

	err = BS->LocateDevicePath(&BlockIoProtocol, &devicePath, &deviceHandle);
	if (err != EFI_SUCCESS)
	{
		return EFI_NOT_FOUND;
	}
	Print(L"Located handle for interface\n");

	err = BS->HandleProtocol(deviceHandle, &BlockIoProtocol, (void**)block);
	if (err != EFI_SUCCESS)
	{
		return EFI_NOT_FOUND;
	}
	Print(L"BlockIoProtocol interface retrieved\n");

	Print(L"Detected MediaID: %d\n", (*block)->Media->MediaId);
	Print(L"Detected Size: %d\n", (*block)->Media->LastBlock * (*block)->Media->BlockSize);
	Print(L"Detected IsLogical: %d\n", (*block)->Media->LogicalPartition);

	return EFI_SUCCESS;
}

EFI_STATUS PerformCopy(SIMPLE_READ_FILE file, EFI_DISK_IO* disk)
{


	return EFI_SUCCESS;
}

// Application entrypoint
EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_INPUT_KEY Key;

	InitializeLib(ImageHandle, SystemTable);
	
	Print(L"Partition Format and Copier helper\n\n");
	
	EFI_STATUS err;
	do
	{
		EFI_HANDLE handle;
		SIMPLE_READ_FILE file;
		err = FindImageFile(L"\\disk.img", &handle, &file);
		if (err != EFI_SUCCESS)
		{
			Print(L"Aborting due to source disk image file not found\n");
			break;
		}

		EFI_DISK_IO* disk;
		EFI_BLOCK_IO* block;
		err = FindTargetDisk(L"BLK0", &block, &disk);
		if (err != EFI_SUCCESS)
		{
			Print(L"Aborting due to target device not found\n");
			break;
		}
		
		PerformCopy(file, disk);

		CloseSimpleReadFile(file);
	} while (0);
	
	Print(L"\nPress any key to exit.\n");
	SystemTable->ConIn->Reset(SystemTable->ConIn, FALSE);
	while (SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key) == EFI_NOT_READY);

#if defined(_DEBUG)
	// If running in debug mode, use the EFI shut down call to close QEMU
	RT->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
#endif

	return EFI_SUCCESS;
}
