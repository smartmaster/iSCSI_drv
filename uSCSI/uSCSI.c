
#include "precomp.h"

//
//
//

NTSTATUS uAddDevice( IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT Pdo)
{
	NTSTATUS				Status;
	PDEVICE_OBJECT			DevFdo;
	PFDO_EXT				FdoExt;
	
	Status = IoCreateDevice(DriverObject, 
   							sizeof(FDO_EXT) ,    				//user size 							   							
   							NULL,   							//Name	
   							FILE_DEVICE_BUS_EXTENDER,      		//Device Type
   							FILE_DEVICE_SECURE_OPEN|
   							FILE_AUTOGENERATED_DEVICE_NAME, 	//Characteristics
   							TRUE,                           	//Exclusive
   							&DevFdo);
   							
	if ( NT_ERROR( Status ) )
		goto ErrorOut;

	FdoExt = DevFdo->DeviceExtension;
	
	FdoExt->Self 	= DevFdo;
	FdoExt->IsFDO 	= TRUE;		
	FdoExt->LowerDev = IoAttachDeviceToDeviceStack( DevFdo , Pdo );
	
	FdoExt->PDOCount = 0;
	KeInitializeSpinLock ( &FdoExt->PDOLock );
	InitializeListHead ( &FdoExt->PDOList );	
	
	Status = IoRegisterDeviceInterface( FdoExt->LowerDev , 
										&USCSI_DISK_INTERFACE,
										NULL,
										&FdoExt->InterfaceName);
	if ( NT_ERROR( Status ) )
		goto ErrorOut;
			
	DevFdo->Flags &= ~DO_DEVICE_INITIALIZING;
	
	return Status;

ErrorOut:

	if ( DevFdo )
	{
		if (FdoExt->LowerDev)
			IoDetachDevice (DevFdo);
		IoDeleteDevice (DevFdo);
	}
	return Status;
}

//
//
//

NTSTATUS DriverEntry(IN PDRIVER_OBJECT 	DriverObject,IN PUNICODE_STRING	RegistryPath)
{
	NTSTATUS	Status = STATUS_SUCCESS;
	
	DriverObject->DriverExtension->AddDevice 			= uAddDevice;
	
	DriverObject->MajorFunction[ IRP_MJ_READ ] = 
		DriverObject->MajorFunction[ IRP_MJ_WRITE ] 	= uReadWrite;
	
	DriverObject->MajorFunction[ IRP_MJ_CREATE ] = 
		DriverObject->MajorFunction[ IRP_MJ_CLOSE ] 	= uCreateClose;

	DriverObject->MajorFunction[ IRP_MJ_SCSI ] 			= uScsi;	
	
	DriverObject->MajorFunction[ IRP_MJ_FLUSH_BUFFERS ] = uFlush;	
	DriverObject->MajorFunction[ IRP_MJ_DEVICE_CONTROL] = uIoCtl;
	DriverObject->MajorFunction[ IRP_MJ_PNP ] 			= uPnP;
	
	return Status;
}