/*
 * Easy UEFI disk writer
 * Copyright © 2015 Jairo Velasco <jairov*@****.com> - Public Domain
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

EFI_STATUS FindTargetDisk(CHAR16* targetDiskPath, EFI_BLOCK_IO** block, EFI_DISK_IO** disk)
{	
	EFI_HANDLE* handleArray;
	UINTN nbHandles;

	// Find Disk IO interfaces
	EFI_STATUS err = BS->LocateHandleBuffer(ByProtocol, &DiskIoProtocol, NULL, &nbHandles, &handleArray);
	if (err != EFI_SUCCESS)
	{
		return EFI_NOT_FOUND;
	}
	Print(L"Located %d handles implementing DiskIoProtocol\n", nbHandles);

	for (UINTN i = 0; i < nbHandles; i++)
	{
		EFI_DEVICE_PATH* devPath = DevicePathFromHandle(handleArray[i]);
		CHAR16* path = DevicePathToStr(devPath);
		if (StrCmp(path, targetDiskPath) != 0)
		{
			Print(L"Discard device: %s\n", path);
			continue;
		}

		// Open the Device IO protocol
		err = BS->HandleProtocol(handleArray[i], &DiskIoProtocol, (void **)disk);
		if (err != EFI_SUCCESS)
		{
			Print(L"Discard device: %s due DiskIoProtocol not present\n", path);
			continue;
		}

		err = BS->HandleProtocol(handleArray[i], &BlockIoProtocol, (void**)block);
		if (err != EFI_SUCCESS)
		{
			Print(L"Discard device due BlockIoProtocol not present: %s\n", path);
			continue;
		}
		
		Print(L"Found device: %s\n", path);
		Print(L" Size: %d\n", (*block)->Media->LastBlock * (*block)->Media->BlockSize);
		Print(L" IsLogical: %d\n", (*block)->Media->LogicalPartition);
		Print(L" MediaId: %d\n", (*block)->Media->MediaId);

		FreePool(handleArray);
		return EFI_SUCCESS;
	}

	FreePool(handleArray);
	return EFI_NOT_FOUND;
}

#define BUFFER_SIZE 4096

EFI_STATUS PerformCopy(SIMPLE_READ_FILE file, EFI_DISK_IO* disk, EFI_BLOCK_IO* block)
{		
	CHAR8 buffer[BUFFER_SIZE];
	UINTN readBytes;
	UINT64 offset = 0;
	EFI_STATUS err;
	
	do
	{
		readBytes = BUFFER_SIZE;
		err = ReadSimpleReadFile(file, offset, &readBytes, buffer);
		if(err != EFI_SUCCESS)
		{
			StatusToString((CHAR16*)buffer, err);
			Print(L"Error reading from image file: %d - %s\n", err, buffer);
			return err;
		}
		
		err = disk->WriteDisk(disk, block->Media->MediaId, offset, readBytes, buffer);
		if (err != EFI_SUCCESS)
		{
			StatusToString((CHAR16*)buffer, err);
			Print(L"Error writing to target disk: %d - %s\n", err, buffer);
			return err;
		}

		offset += readBytes;

		if (offset % (BUFFER_SIZE * 20) == 0)
		{
			Print(L"Copy progress: %d bytes\n", offset);
		}
	} while (readBytes == BUFFER_SIZE);

	err = block->FlushBlocks(block);
	if (err != EFI_SUCCESS)
	{
		StatusToString((CHAR16*)buffer, err);
		Print(L"Error flushing buffer to target disk: %d - %s\n", err, buffer);
		return err;
	}

	Print(L"Copy finished, total %d bytes\n", offset);

	return EFI_SUCCESS;
}

// Application entrypoint
EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_INPUT_KEY Key;

	InitializeLib(ImageHandle, SystemTable);
	
	Print(L"Partition Format and Copier helper\n\n");

	CHAR16** argv;
	INTN argc = GetShellArgcArgv(ImageHandle, &argv);
	
	EFI_STATUS err;
	do
	{
		Print(L"ArgCount: %d\n", argc);
		if(argc < 3)
		{
			Print(L"Invalid argument count\n");
			break;
		}

		CHAR16* imageFile = argv[1];
		CHAR16* targetDisk = argv[2];

		Print(L"Disk image file: %s\n", imageFile);
		Print(L"Target device: %s\n", targetDisk);

		EFI_HANDLE handle;
		SIMPLE_READ_FILE file;
		err = FindImageFile(imageFile, &handle, &file);
		if (err != EFI_SUCCESS)
		{
			Print(L"Aborting due to source disk image file not found\n");
			break;
		}

		EFI_DISK_IO* disk;
		EFI_BLOCK_IO* block;
		err = FindTargetDisk(targetDisk, &block, &disk);
		if (err != EFI_SUCCESS)
		{
			Print(L"Aborting due to target device not found\n");
			break;
		}
		
		PerformCopy(file, disk, block);

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
