#define _NTDDK_

#include "windows.h"
#include "ntstatus.h"
#pragma warning(disable : 4200)

typedef LONG		NTSTATUS;
typedef NTSTATUS	*PNTSTATUS;
typedef NTSTATUS	(NTAPI *NTSYSCALL)();
typedef NTSYSCALL	*PNTSYSCALL;
typedef ULONG		KAFFINITY;
typedef KAFFINITY	*PKAFFINITY;
typedef LONG		KPRIORITY;
typedef BYTE		KPROCESSOR_MODE;
typedef VOID		*POBJECT;
typedef VOID		(*PKNORMAL_ROUTINE) (
						IN PVOID NormalContext,
						IN PVOID SystemArgument1,
						IN PVOID SystemArgument2
					);

typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength), length_is(Length) ]
#endif // MIDL_PASS
    PCHAR Buffer;
} STRING, *PSTRING;


typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;

typedef STRING OEM_STRING;
typedef PSTRING POEM_STRING;

typedef struct _HARDWARE_PTE
{
    ULONG Valid             : 1;
    ULONG Write             : 1;
    ULONG Owner             : 1;
    ULONG WriteThrough      : 1;
    ULONG CacheDisable      : 1;
    ULONG Accessed          : 1;
    ULONG Dirty             : 1;
    ULONG LargePage         : 1;
    ULONG Global            : 1;
    ULONG CopyOnWrite       : 1;
    ULONG Prototype         : 1;
    ULONG reserved          : 1;
    ULONG PageFrameNumber   : 20;
} HARDWARE_PTE, *PHARDWARE_PTE;

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES
{
	ULONG			uLength;
	HANDLE			hRootDirectory;
	PUNICODE_STRING	pObjectName;
	ULONG			uAttributes;
	PVOID			pSecurityDescriptor;
	PVOID			pSecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _CLIENT_ID
{
	HANDLE	UniqueProcess;
	HANDLE	UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _PEB_FREE_BLOCK
{
   struct _PEB_FREE_BLOCK	*Next;
   ULONG					Size;
} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;

typedef struct _CURDIR
{
   UNICODE_STRING	DosPath;
   HANDLE			Handle;
} CURDIR, *PCURDIR;

typedef struct _RTL_DRIVE_LETTER_CURDIR
{
	WORD	Flags;
	WORD	Length;
	DWORD	TimeStamp;
	STRING	DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

#define	PROCESS_PARAMETERS_NORMALIZED	1	// pointers in are absolute (not self-relative)

typedef struct _PROCESS_PARAMETERS
{
    ULONG					MaximumLength;
    ULONG					Length;
    ULONG					Flags;				// PROCESS_PARAMETERS_NORMALIZED
    ULONG					DebugFlags;
    HANDLE					ConsoleHandle;
    ULONG					ConsoleFlags;
    HANDLE					StandardInput;
    HANDLE					StandardOutput;
    HANDLE					StandardError;
    CURDIR					CurrentDirectory;
    UNICODE_STRING			DllPath;
    UNICODE_STRING			ImagePathName;
    UNICODE_STRING			CommandLine;
    PWSTR					Environment;
    ULONG					StartingX;
    ULONG					StartingY;
    ULONG					CountX;
    ULONG					CountY;
    ULONG					CountCharsX;
    ULONG					CountCharsY;
    ULONG					FillAttribute;
    ULONG					WindowFlags;
    ULONG					ShowWindowFlags;
    UNICODE_STRING			WindowTitle;
    UNICODE_STRING			Desktop;
    UNICODE_STRING			ShellInfo;
    UNICODE_STRING			RuntimeInfo;
	RTL_DRIVE_LETTER_CURDIR	CurrentDirectores[32];
} PROCESS_PARAMETERS, *PPROCESS_PARAMETERS;

typedef struct _RTL_BITMAP
{
	DWORD	SizeOfBitMap;
	PDWORD	Buffer;
} RTL_BITMAP, *PRTL_BITMAP, **PPRTL_BITMAP;

#define LDR_STATIC_LINK				0x0000002
#define LDR_IMAGE_DLL				0x0000004
#define LDR_LOAD_IN_PROGRESS		0x0001000
#define LDR_UNLOAD_IN_PROGRESS		0x0002000
#define LDR_ENTRY_PROCESSED			0x0004000
#define LDR_ENTRY_INSERTED			0x0008000
#define LDR_CURRENT_LOAD			0x0010000
#define LDR_FAILED_BUILTIN_LOAD		0x0020000
#define LDR_DONT_CALL_FOR_THREADS	0x0040000
#define LDR_PROCESS_ATTACH_CALLED	0x0080000
#define LDR_DEBUG_SYMBOLS_LOADED	0x0100000
#define LDR_IMAGE_NOT_AT_BASE		0x0200000
#define LDR_WX86_IGNORE_MACHINETYPE	0x0400000

typedef struct _LDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY		InLoadOrderModuleList;
    LIST_ENTRY		InMemoryOrderModuleList;
    LIST_ENTRY		InInitializationOrderModuleList;
    PVOID			DllBase;
    PVOID			EntryPoint;
    ULONG			SizeOfImage;	// in bytes
    UNICODE_STRING	FullDllName;
    UNICODE_STRING	BaseDllName;
    ULONG			Flags;			// LDR_*
    USHORT			LoadCount;
    USHORT			TlsIndex;
    LIST_ENTRY		HashLinks;
    PVOID			SectionPointer;
    ULONG			CheckSum;
    ULONG			TimeDateStamp;
//    PVOID			LoadedImports;					// seems they are exist only on XP !!!
//    PVOID			EntryPointActivationContext;	// -same-
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _PEB_LDR_DATA {
	ULONG Length;
	BOOLEAN Initialized;
	PVOID SsHandle;
	LIST_ENTRY ModuleListLoadOrder;
	LIST_ENTRY ModuleListMemoryOrder;
	LIST_ENTRY ModuleListInitOrder;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef VOID NTSYSAPI (*PPEBLOCKROUTINE)(PVOID);

typedef struct _SYSTEM_STRINGS
{
	UNICODE_STRING	SystemRoot;       // C:\WINNT
	UNICODE_STRING	System32Root;     // C:\WINNT\System32
	UNICODE_STRING	BaseNamedObjects; // \BaseNamedObjects
}SYSTEM_STRINGS,*PSYSTEM_STRINGS;

typedef struct _TEXT_INFO
{
	PVOID			Reserved;
	PSYSTEM_STRINGS	SystemStrings;
}TEXT_INFO, *PTEXT_INFO;

typedef struct _PEBME
{
	UCHAR				InheritedAddressSpace;				// 0
	UCHAR				ReadImageFileExecOptions;			// 1
	UCHAR				BeingDebugged;						// 2
	BYTE				b003;								// 3
	PVOID				Mutant;								// 4
	PVOID				ImageBaseAddress;					// 8
	PPEB_LDR_DATA		Ldr;								// C
	PPROCESS_PARAMETERS	ProcessParameters;					// 10
	PVOID				SubSystemData;						// 14  
	PVOID				ProcessHeap;						// 18
	KSPIN_LOCK			FastPebLock;						// 1C
	PPEBLOCKROUTINE		FastPebLockRoutine;					// 20
	PPEBLOCKROUTINE		FastPebUnlockRoutine;				// 24
	ULONG				EnvironmentUpdateCount;				// 28
	PVOID				*KernelCallbackTable;				// 2C
	PVOID				EventLogSection;					// 30
	PVOID				EventLog;							// 34
	PPEB_FREE_BLOCK		FreeList;							// 38
	ULONG				TlsExpansionCounter;				// 3C
	PRTL_BITMAP			TlsBitmap;							// 40
	ULONG				TlsBitmapData[0x2];					// 44
	PVOID				ReadOnlySharedMemoryBase;			// 4C
	PVOID				ReadOnlySharedMemoryHeap;			// 50
	PTEXT_INFO			ReadOnlyStaticServerData;			// 54
	PVOID				InitAnsiCodePageData;				// 58
	PVOID				InitOemCodePageData;				// 5C
	PVOID				InitUnicodeCaseTableData;			// 60
	ULONG				KeNumberProcessors;					// 64
	ULONG				NtGlobalFlag;						// 68
	DWORD				d6C;								// 6C
	LARGE_INTEGER		MmCriticalSectionTimeout;			// 70
	ULONG				MmHeapSegmentReserve;				// 78
	ULONG				MmHeapSegmentCommit;				// 7C
	ULONG				MmHeapDeCommitTotalFreeThreshold;	// 80
	ULONG				MmHeapDeCommitFreeBlockThreshold;	// 84
	ULONG				NumberOfHeaps;						// 88
	ULONG				AvailableHeaps;						// 8C
	PHANDLE				ProcessHeapsListBuffer;				// 90
	PVOID				GdiSharedHandleTable;				// 94
	PVOID				ProcessStarterHelper;				// 98
	PVOID				GdiDCAttributeList;					// 9C
	KSPIN_LOCK			LoaderLock;							// A0
	ULONG				NtMajorVersion;						// A4
	ULONG				NtMinorVersion;						// A8
	USHORT				NtBuildNumber;						// AC
	USHORT				NtCSDVersion;						// AE
	ULONG				PlatformId;							// B0
	ULONG				Subsystem;							// B4
	ULONG				MajorSubsystemVersion;				// B8
	ULONG				MinorSubsystemVersion;				// BC
	KAFFINITY			AffinityMask;						// C0
	ULONG				GdiHandleBuffer[0x22];				// C4
	ULONG				PostProcessInitRoutine;				// 14C
	ULONG				TlsExpansionBitmap;					// 150
	UCHAR				TlsExpansionBitmapBits[0x80];		// 154
	ULONG				SessionId;							// 1D4
	ULARGE_INTEGER		AppCompatFlags;						// 1D8
	PWORD				CSDVersion;							// 1E0
/*	PVOID				AppCompatInfo;						// 1E4
	UNICODE_STRING		usCSDVersion;
	PVOID				ActivationContextData;
    PVOID				ProcessAssemblyStorageMap;
    PVOID				SystemDefaultActivationContextData;
    PVOID				SystemAssemblyStorageMap;
    ULONG				MinimumStackCommit; */
} PEBME, *PPEBME;

