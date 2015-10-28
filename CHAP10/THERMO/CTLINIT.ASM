; Controls DLL Initialization 
; Define the initialization/termination entry point for the Controls DLL

	.386p

ifdef __TASM__
model flat,pascal
endif

_TEXT	SEGMENT	DWORD USE32 PUBLIC 'CODE'
	EXTRN	_InitializeControlDll:near
       
ifdef __MASM386__
	ASSUME   CS: FLAT, DS: FLAT, ES: FLAT
endif

_DLLEntry:
        jmp _InitializeControlDll

_TEXT	ENDS                              
	END	_DLLEntry
