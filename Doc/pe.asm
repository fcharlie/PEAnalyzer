
;; CONSTANTS
; Number of bytes in the last 512 bytes page to be loaded by DOS
$LAST_PAGE_BYTES            equ     EXE_HEADER - DOS_HEADER
; The content of each section starts at an address that is a multiple of this value
$SECTION_ALIGNMENT          equ     4096
; Each image section is aligned to this boundary
$FILE_ALIGNMENT             equ     512
; Preferred absolute address where the application will be loaded into memory (4MB)
$PREFERRED_ADDRESS          equ     4194304
; Sections count inside the image
$SECTIONS_COUNT             equ     4
; Real size of the headers
$HEADERS_SIZE               equ     HEADERS_END - DOS_HEADER
; The real size of the code section
$CODE_SIZE                  equ     CODE_END - CODE
; Real size of the data section
$DATA_SIZE                  equ     DATA_END - INIT_DATA
; Imports sections real size
$IMPORTS_SIZE               equ     IMPORTS_END - IMPORT_DATA
; Imported elements table size
$IMPORTS_TABLE_SIZE         equ     KERNEL32_ITABLE_END - KERNEL32_ITABLE
; Size of the uninitialized data in memory
$UDATA_SIZE                 equ     16

;; DEFINITIONS
; Align to the specified boundary
%define Round(Number, Boundary)     (Number + Boundary - 1)/Boundary * Boundary
; Calculate the relative virtual address at the beginning of the sections
%define RVA(BaseAddress)            (BaseAddress - CODE)/$FILE_ALIGNMENT * $SECTION_ALIGNMENT + \
                                    $SECTION_ALIGNMENT
; Get the RVA of the specified base address
%define RVA(Adress, BaseAddress)    RVA(BaseAddress) + (Adress - BaseAddress)
; Size of the image in memory (everything aligned to $ALINIERE_SECTIUNE)             ;
%define ImageSize                   RVA(IMAGE_END) + Round($UDATA_SIZE, $SECTION_ALIGNMENT)

;; MACROS
; Add the value in the first argument as many times as the second arguments specifies
%macro Fill 2
    times %1 db %2
%endmacro

;; EXECUTABLE HEADERS
; Any valid PE file will begin with the DOS header, for backward-compatibility with DOS
DOS_HEADER: 
    ; At the beginning we will add the MZ (Mark Zbikowski) signature to keep the compatibility 
    ; with MS-D0S (16 bytes). If the application will run in DOS will display a message about the 
    ; application compatible only with Windows 
    .MZSignature            db          "MZ"
    ; The number of bytes in the last page of 512 bytes that DOS will upload. Last page usually does 
    ; not contain exactly 512 bytes, so we will specify the exact size here, which is the 
    ; DOS header size plus the portion of code that will be executed in DOS to display the message 
    ; of incompatibility with it
    .LastPageBytes          dw          $LAST_PAGE_BYTES
    ; Pages to be loaded into memory DOS. It is a single page from which will be loaded only the 
    ; bytes specified in LastPageBytes. The module's size in DOS is determined using the following
    ; formula:
    ; ((TotalPages * 512) - (DOSHeaderSize * 16)) - LastPageBytes
    .TotalPages             dw          1
    ; Number of memory reallocation. There is no relocation, so the number of relocations is 
    ; 0. In DOS we can specify an exact address for different segments of the file but in this 
    ; case there is no need
    .Relocations            dw          4
    ; MZ header size in paragraphs (16 bytes). Immediately after this, next portion is DOS 
    ; with 16-byte code that runs when the application is open in DOS;
    .DOSHeaderSize          dw          4
    ; Minimum amount of paragraphs required to be loaded in memory besides code. If in memory there are not 
    ; at least as many paragraphs as specified here, the program won't run
    .MinimumParagraphs      dw          0
    ; Maximum of paragraphs that can be loaded by DOS in memory, besides code. The system will 
    ; provide as much additional memory as needed, so we will not limit it
    .MaximumParagraphs      dw          65535
    ; Initial value of the regiter that stores the memory stacks's segment. This value is added
    ; to the segment where the program is loaded and the result is stored in the SS register
    .SSRegister             dw          0
    ; Initial value of the register containing the stack pointer (its initial size)
    .SPRegister             dw          0x00B8
    ; Checksum. Suma de control. The inverted checksum of all the words in the file. Usually ignored 
    ; by the loader
    .ChecksUM               dw          0
    ; Initial instruction pointer value
    .IPRegister             dw          0
    ; Initial (relative) code segment value
    .CSRegister             dw          0
    ; The address to the reallocations table (if this does exist). This address is relative to 
    ; the beginning of the file. As this file will not run in DOS, there are no relocations. We 
    ; specify this by assigning it the value '40h'
    .RealocationsTable      dw          0x0040
    ; Overlays number. We will not use overlays. This is the only one and the main program to load 
    ; in memory
    .Overlays               dw          0
    ; Fill with 8 bytes
    .RezervedWords          dq          0
    ; OEM Identifier
    .OEMIdentifier          dw          0
    ; OEM Informations
    .OEMInfo                dw          0
    ; Add 10 null bytes
    Fill                    20,         0
    ; PE Header address, where the application will run in Windows mode
    .PEHeader               dd          EXE_HEADER

; A portion of code to show a message about incompatibility in DOS mode
DOS_PROGRAM:
    ; Move code segment in data segment
    push                    cs
    ; Store the data after the code, to save space
    pop                     ds
    ; Load the pointer to the message that will be shown on the screen
    mov                     dx,         DOSMessage - DOS_PROGRAM
    ; This is the argument for the output operation of the 21h interrupt
    mov                     ah,         9
    ; Call the interrupt and show the message
    int                     21h
    ; Exit argument for the same interrupt
    mov                     ax,         0x4C01
    ; Exit
    int                     21h
    
    ; Message to show in DOS when this program is executed
    DOSMessage:
        db                  "This program wasn't created for your system!", 0Dh, 0Dh, 0Ah, '$'
    
    ; Fill with zeros up to the total of 64 bytes of this DOS portion
    Fill                   64-$+DOS_PROGRAM, 0

; Here we will define the PE header, which will validate this file as a Portable Executable
EXE_HEADER:
    ; As in the DOS header, we will begin with a signature identifying this as a portable executable
    .PESignature            db          "PE", 0, 0
    ; The type of processor for which this file is intended, in our case Intel i386
    .Microprocessor         dw          0x014C
    ; Sections count in this file
    .SectionsCount          dw          $SECTIONS_COUNT
    ; Header date and hour of creation
    .CreationDateHour       dd          1371668450
    ; Pointer to symbols table. The symbols will help the compiler identify different elements 
    ; inside the source code. Don't need it now
    .SymbolsTable           dd          0
    ; How many symbols we have in the table. No symbols table, nothing
    .SymbolsCount           dd          0
    ; The size of the additional header below this one
    .OptionalHeaderSize     dw          SECTIONS_HEADER_TABLE - OPTIONAL_HEADER
    ; File characteristics (see reference)
    .Characteristics        dw          0x0002|0x0004|0x0008|0x0100|0x0200

; Optional header (in fact, obligatory) will let us know the structure of this executable
OPTIONAL_HEADER:
    ; Just as before, we will start with a number that identifies and validates this header
    .OptHeaderSignature     dw          0x010B
    ; The major version of the linker.  This program doesn't have such a thing
    .LinkerMajorVersion     db          0
    ; The minor version of the linker
    .LinkerMinorVersion     db          0
    ; The size of the code sections in the image
    .CodeSize               dd          Round($CODE_SIZE, $SECTION_ALIGNMENT)
    ; The size of the initialized data sections
    .DataSize               dd          Round($DATA_SIZE, $SECTION_ALIGNMENT)
    ; The size of the uninitialized (BSS) data sections. These sections do not occupy any space 
    ; inside the file but the system will allocate the necessary memory amount at run-time
    .UdataSize              dd          Round($UDATA_SIZE, $SECTION_ALIGNMENT)
    ; The relative address of the entry point of the application. As this is found in the first
    ; section of the memory and it starts at $SECTION_ALIGNMENT, the entry point address is the 
    ; same $SECTION_ALIGNMENT
    .EntryPoint             dd          RVA(CODE)
    ; The relative virtual address (RVA) of the code section
    .CodeBase               dd          RVA(CODE)
    ; The RVA of the data section
    .DataBase               dd          RVA(INIT_DATA)
    ; The preferred address for the image to be loaded at. For EXE image, this is 0x00400000, for 
    ; DLL file it is 0x10000000. On top of this address the code and data base above is added
    .MemoryImageBase        dd          $PREFERRED_ADDRESS
    ; Section alignment. Default value is 0x1000 (4096). Each section starts at a virtual address
    ; multiple of this value
    .SectionAlignment       dd          $SECTION_ALIGNMENT
    ; File alignment. Recommended value is 0x0200. Each section inside the file is aligned to this 
    ; value
    .FileAlignment          dd          $FILE_ALIGNMENT
    ; Indicates the major version of the minimum accepted operating system. In our case, this can
    ; run on Windows 95 and newer versions
    .MajorOSVersion         dw          4
    ; The minor version of the operating system
    .MinorOSVersion         dw          0
    ; Image major version
    .MajorImageVersion      dw          0
    ; Image minor version
    .MinorImageVersion      dw          0
    ; The major version of the sub-system. Minimum accepted value is 3
    .MajorSubsysVersion     dw          3
    ; The minor version of the sub-system. Minimum accepted value is 10
    .MinorSubsysVersion     dw          10
    ; WIN32 version, always 0
    .WIN32Version           dd          0
    ; File image size, including headers and aligned to $SECTION_ALIGNMENT
    .ImgSize                dd          ImageSize
    ; Size of all the headers, including the sections headers, aligned to $FILE_ALIGNMENT
    .HeadersSize            dd          Round($HEADERS_SIZE, $FILE_ALIGNMENT)
    ; Checksum that validates the integrity of the image. Usually, 0
    .Checksum               dd          0
    ; The subsystem used for the user interface, in our case, the console
    .InterfaceSubsystem     dw          3
    ; DLL characteristics. No DLL, so 0
    .DLLAttributes          dw          0
    ; The amount of memory reserved by the system for the stack
    .ReservedStack          dd          4096
    ; The initial amount of memory reserved by the system for the local stack
    .LocalStack             dd          4096
    ; Free memory initial size
    .ReservedFreeMemory     dd          65536
    ; Free initial memory for the image
    .FreeMemory             dd          0
    ; Debugging pointer, obsolete
    .DebuggingPointer       dd          0 
    ; Entries amount in the DATA_TABLE. 16 is used most of the times
    .DataDirectories        dd          16
    ; Address of the exported elements table. This is not a DLL, nothing is exported
    .ExportTable            dd          0
    ; The size of the exported elements table
    .ExportTableSize        dd          0
    ; The address of the imported elements table, where we have jumps to other modules' elements
    .ImportTable            dd          RVA(KERNEL32_ITABLE, IMPORT_DATA)
    ; Size of the imported elements table
    .ImportTableSize        dd          $IMPORTS_TABLE_SIZE
    ; 112 bytes reserved
    Fill                    112,        0

; This table contains the headers of the sections inside this image, and their attributes
SECTIONS_HEADER_TABLE:
    ; The header of the code section, where the executable code is located. Usuallt, this section 
    ; is called text, but we will call it 'code'
    CODE_SECTION_HEADER:
        ; Name of this section. Can't have more than 8 characters
        .Name               db          ".code", 0, 0, 0
        ; Virtual size (in memory) of this section. If it is bigger than the size on disk, will be 
        ; filled with 0
        .SectionSize        dd          Round($CODE_SIZE, $SECTION_ALIGNMENT)
        ; RVA of this section
        .SectionStart       dd          RVA(CODE)
        ; Aligned real size of this section
        .RealSize           dd          Round($CODE_SIZE, $SECTION_ALIGNMENT)
        ; Real start address
        .StartAddress       dd          CODE
        ; Relocation table address
        .RelocationsTable   dd          0
        ; A file pointer to the beginning of the line-number entries for the section. 
        ; If there are no COFF line numbers, this value is zero
        .LineNumbers        dd          0
        ; Relocations amount
        .RelocationsTotal   dw          0
        ; The number of line-number entries for the section
        .LineNumbersTotal   dw          0
        ; Characteristics of this section. It can be read, written and executed
        .Characteristics    dd          0x00000020|0x20000000|0x40000000|0x80000000
    
    ; Imports section header
    ANTET_SECTIUNE_IMPORT:
        .Name               db          ".idat", 0, 0, 0
        .SectionSize        dd          Round($IMPORTS_SIZE, $SECTION_ALIGNMENT)
        .SectionStart       dd          RVA(IMPORT_DATA)
        .RealSize           dd          Round($IMPORTS_SIZE, $FILE_ALIGNMENT)
        .StartAddress       dd          IMPORT_DATA
        .RelocationsTable   dd          0
        .LineNumbers        dd          0
        .RelocationsTotal   dw          0
        .LineNumbersTotal   dw          0
        .Characteristics    dd          0x00000040|0x40000000|0x80000000 
    
    ; Initialized data section header
    DATA_SECTION_HEADER:
        .Name               db          ".data", 0, 0, 0
        .SectionSize        dd          Round($DATA_SIZE, $SECTION_ALIGNMENT)
        .SectionStart       dd          RVA(INIT_DATA)
        .RealSize           dd          Round($DATA_SIZE, $FILE_ALIGNMENT)
        .StartAddress       dd          INIT_DATA
        .RelocationsTable   dd          0
        .LineNumbers        dd          0
        .RelocationsTotal   dw          0
        .LineNumbersTotal   dw          0
        .Characteristics    dd          0x00000040|0x40000000|0x80000000

    ; Uninitialized data section header
    NULL_DATA_HEADER:
        .Name               db          ".null", 0, 0, 0
        .SectionSize        dd          Round($UDATA_SIZE, $SECTION_ALIGNMENT)
        .SectionStart       dd          RVA(NULL_DATA)
        .RealSize           dd          0
        .StartAddress       dd          0
        .RelocationsTable   dd          0
        .LineNumbers        dd          0
        .RelocationsTotal   dw          0
        .LineNumbersTotal   dw          0
        .Characteristics    dd          0x00000080|0x40000000|0x80000000    
            
;; Here is where all the headers declarations ends
HEADERS_END:
        
;; Align to page size
align   $FILE_ALIGNMENT

;; CODE SECTION
; For 32-bits processors
use32

; The RVA of the global uninitialized variables, from the BSS section. The first vairable will 
; contain the handler of the output device
OutputHandler       equ RVA(NULL_DATA) + 0
; This will store the amount of characters WriteConsoleW outputs
WrittenChars        equ OutputHandler + 4

; All the code to be run is here
CODE:
    ; In first place, we will need a handler to the console
    call    GetOutputHandler
    ; Add the message to be shown to the stack
    push    dword       $PREFERRED_ADDRESS + RVA(HelloMessage, INIT_DATA)
    ; The amount of characters to be shown
    push    dword       13
    ; Call the function that will display the text
    call    ShowText
    ; Finish application properly
    jmp     CloseApplication
  
    ; This portion of code will get a handler to the console and will store it in OutputHandler
    GetOutputHandler:
        ; The argument indicating what we want, which is an output device
        push                    -11
        ; Now, call the function from Kernel32.dll
        call                    dword [$PREFERRED_ADDRESS + RVA(F_GetStdHandle, IMPORT_DATA)]
        ; If the inverted value is smaller than 1, we have a problem and the application must exit
        cmp                     eax,    1
        ; Store the BX register in order to restore it in case of success
        push                    ebx
        ; If there's an error, we will force the end of the process and the exit code will be 1
        mov                     ebx,    1
        ; Jump to the function that ends the application
        jl                      CloseApplication
        ; Restore the BX register, before receiving the exit code
        pop                     ebx
        ; Save the handler in BSS
        mov                     dword [$PREFERRED_ADDRESS + OutputHandler], eax
        ; Return to the place from which this function has been called
        ret

    ; This function will show a text on the console
    ShowText:
        ; Save EBP register in order to restore it later
        push                    ebp
        ; Move ESP in EBP in order to read the received arguments
        mov                     ebp,    esp
        ; Reserved NULL argument for the WriteConsoleW function
        push                    0
        ; This is where the amount of written characters will be saved
        push                    dword $PREFERRED_ADDRESS + WrittenChars
        ; The amount of characters to be written, from the second argument
        push                    dword [ebp + 8]
        ; The pointer to the text, from the first argument on the stack
        push                    dword [ebp + 12]
        ; The handler to the output device
        push                    dword [$PREFERRED_ADDRESS + OutputHandler]
        ; Call the function that will display the text
        call                    dword [$PREFERRED_ADDRESS + RVA(F_WriteConsoleW, IMPORT_DATA)]
        ; Restore EBP
        pop     ebp
        ; Return and at the same time restore the stack
        ret     8
    
    ; This function will finish the application and return the exit code in EBX
    CloseApplication:
        ; Store the BX value on the stack, it is the exit code
        push                    ebx
        ; Call the function that will exit the application with the code in EBX
        call                    dword [$PREFERRED_ADDRESS + RVA(F_ExitProcess, IMPORT_DATA)]
        ; If this didn't work, the stack is restored and we just return
        pop                     ebx
        ret
    
    ; Code section is aligned to 512 bytes
    align   $FILE_ALIGNMENT
CODE_END:

; Here we have all the data and functions imported from dynamic libraries
IMPORT_DATA:
    ; This is the main library in Windows and will provide essential functions
    KERNEL32_LIBRARY            db         'kernel32.dll', 0
    ; Here we describe the characteristics of the imports table for the KERNEL32.DLL library
    KERNEL32_ITABLE:  
        .originalfthk           dd          0
        .timedate               dd          0  
        .forwarder              dd          0  
        .name                   dd          RVA(KERNEL32_LIBRARY, IMPORT_DATA)
        .firstthunk             dd          RVA(IMPORTED_FUNCTIONS, IMPORT_DATA)
    
        ; The end of the table fir the KERNEL32.DLL library
        Fill                    20,         0
    KERNEL32_ITABLE_END:
    
    ; Imported elements from the library above
    KERNEL32_IMPORTED_ELEMENTS:
        I_WriteConsoleW:    
            dw                  0  
            db                  'WriteConsoleW', 0
            align               2  
        Import_GetStdHandle:
            dw                  0  
            db                  'GetStdHandle', 0
            align               2
        I_ExitProcess:
            dw                  0
            db                  'ExitProcess', 0
    
    ; Here we have the addresses of the imported functions
    IMPORTED_FUNCTIONS:
        ; Jump to the function that displays a text on the console
        F_WriteConsoleW:        dd          RVA(I_WriteConsoleW, IMPORT_DATA)
        ; Retrieves a handle to the specified standard device
        F_GetStdHandle:         dd          RVA(Import_GetStdHandle, IMPORT_DATA)
        ; This function closes the process and returns an exit code
        F_ExitProcess           dd          RVA(I_ExitProcess, IMPORT_DATA)
        ; Reserved bytes
        ReservedBytes           dd          0
            
    align   $FILE_ALIGNMENT
IMPORTS_END:

; Initialized data, usually constants like static text messages
INIT_DATA:
    ; Hello message to display
    HelloMessage:    dw          __utf16__("Hello world!"), 0Ah
    ; Alignment to 512 bytes
    align   $FILE_ALIGNMENT
DATA_END:

; BSS section, with uninitialised data
NULL_DATA:

; Image end
IMAGE_END: