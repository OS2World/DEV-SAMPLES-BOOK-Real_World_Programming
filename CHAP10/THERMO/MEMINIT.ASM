; Memory DLL Initialization 
; Define the initialization/termination entry point for the Memory DLL

	.386p

ifdef __TASM__
model flat,pascal
endif

_TEXT	SEGMENT	DWORD USE32 PUBLIC 'CODE'
	EXTRN	_InitializeMemDll:near

ifdef __MASM386__
	ASSUME   CS: FLAT, DS: FLAT, ES: FLAT
endif

_DLLEntry:
        jmp _InitializeMemDll

_TEXT   ENDS
	END	_DLLEntry
