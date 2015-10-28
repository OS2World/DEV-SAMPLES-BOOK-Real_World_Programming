;---------------------------------------------------------------------
;                        Thunks Assembler File
;                             Chapter 13
;
;                    Real World Programming for OS/2
;             Copyright (c) 1993 Blain, Delimon, and English
;---------------------------------------------------------------------
;
; THUNKASM.ASM
;
; Generic thunk code for calling from 32-bit code to 16-bit code
;
; Each 16-bit function to be called is defined here with the _THUNK_ prefix
; Expected calling convention for _THUNK_... functions is _syscall
;
; Each _THUNK_... function re-pushes the parameters onto the stack using
; word alignment.  Parameters which are addresses or fields within structures 
; which are addresses are converted and pushed onto the stack.  All parameters
; are pushed onto the stack using the pascal calling convention (left to right).
;
; _DO_THUNK is the generic thunk function
;

	TITLE	THUNKASM

	.386P

_TEXT32	SEGMENT DWORD USE32 PUBLIC 'CODE'
EXTRN   Dos32FlatToSel:NEAR
EXTRN   Dos32SelToFlat:NEAR
_TEXT32 ENDS

_TEXT16 SEGMENT WORD USE16 PUBLIC 'CODE'
EXTRN   GetBookInformation:FAR
EXTRN   GetAuthorByIndex:FAR
_TEXT16 ENDS

PUBLIC  _THUNK_GetBookInformation
PUBLIC  _THUNK_GetAuthorByIndex

;------------------------------------------------------------------------
;---                       32-BIT CODE SEGMENT                        ---
;------------------------------------------------------------------------

_TEXT32	SEGMENT  
	ASSUME   CS: FLAT, DS: FLAT, ES: FLAT

_THUNK_GetBookInformation	PROC NEAR
                                        ; ---   save the registers    ---
	push 	ebp                     ; push stack frame
	mov  	ebp, esp                ; set new stack frame

                                        ; --- push function parameters---
        mov     eax, DWORD PTR [ebp+8]  ; move 1st parameter to eax
        call    Dos32FlatToSel          ; convert from 32-bit to 16:16
        push    eax                     ; push 1st parameter (16:16)

                                        ; --- push 16-bit function    ---
        mov     eax, SEG GetBookInformation
        shl     eax, 16
        or      eax, OFFSET GetBookInformation
        push    eax

	mov  	ecx, 4                  ; set size of parameters in bytes

        call    _DO_THUNK               ; thunk to the 16-bit function

        mov     ecx, DWORD PTR [ebp+8]  ; move address of structure to ecx
        mov     eax, DWORD PTR [ecx+2]  ; move address of field to eax
        call    Dos32SelToFlat          ; convert structure field address
        mov     DWORD PTR [ecx+2], eax  ; move back info structure field

        leave
        ret
_THUNK_GetBookInformation	ENDP

_THUNK_GetAuthorByIndex 	PROC NEAR
                                        ; ---   save the registers    ---
	push 	ebp                     ; push stack frame
	mov  	ebp, esp                ; set new stack frame

                                        ; --- push function parameters---
        push    WORD PTR [ebp+8]        ; push 1st parameter (usIndex)

                                        ; --- push 16-bit function    ---
        mov     eax, SEG GetAuthorByIndex
        shl     eax, 16
        or      eax, OFFSET GetAuthorByIndex
        push    eax

	mov  	ecx, 2                  ; set size of parameters in bytes

        call    _DO_THUNK               ; thunk to the 16-bit function

        call    Dos32SelToFlat          ; convert returned address
    
        leave
        ret
_THUNK_GetAuthorByIndex   	ENDP

_DO_THUNK PROC NEAR
                                        ; ---   save the registers    ---
	push 	ebp                     ; push stack frame
	mov  	ebp, esp                ; set new stack frame
	push 	ebx                     ; save the ebx register
	push 	edi                     ; save the edi register
	push 	esi                     ; save the esi register
	push 	es                      ; save the es  register
	mov  	eax, esp                ; save current 32-bit stack
	push 	ss                      ; save the ss  register
	push 	eax                     ; save the current stack pointer
                                        
                                        ; ---   copy the paramters    ---       
	lea  	esi, DWORD PTR [ebp+12] ; parameters are at ebp+12
	sub  	esp, ecx                ; ecx contains size of parameters
	mov  	edi, esp                ; set source address
	rep  	movsb                   ; copy the parameters

                                        ; --- convert stack to 16:16  ---
	mov  	eax, esp                ; copy current stack pointer
	ror  	eax, 16                 ; swap high and low word
	shl  	ax, BYTE PTR 3          ; shift left 3 bits
	or   	al, 7                   ; add in the 3 low bits
	rol  	eax, 16                 ; swap high and low word back
	push 	eax                     ; push 16:16 stack pointer
	lss  	sp, DWORD PTR [esp]     ; convert stack
	movzx	esp,sp		        ; insure high word is 0

                                        ; --- call the 16:16 function ---
	jmp	FAR PTR _CALL_THUNK     ; jump to 16-bit segment

_CALL_THUNK_RETURN:                     ; return value in ax and dx
	shl  	eax, 16                 ; shift low word of return
	shrd 	eax,edx,16              ; shift high and low word of return
	movzx	esp, sp                 ; insure high word is 0
	lss  	esp, FWORD PTR [esp]    ; convert stack back
	pop  	es                      ; restore the es register
	pop  	esi                     ; restore the esi register
	pop  	edi                     ; restore the edi register
	pop  	ebx                     ; restore the ebx register
        leave                            
        ret                             
_DO_THUNK ENDP

_TEXT32    ENDS

;------------------------------------------------------------------------
;---                       16-BIT CODE SEGMENT                        ---
;------------------------------------------------------------------------

_TEXT16	SEGMENT  WORD USE16 PUBLIC 'CODE'

_CALL_THUNK	PROC FAR
	call    DWORD PTR [bp+8]       ; call 16:16 function
	jmp	FAR PTR FLAT:_CALL_THUNK_RETURN
_CALL_THUNK     ENDP

_TEXT16    ENDS

END
