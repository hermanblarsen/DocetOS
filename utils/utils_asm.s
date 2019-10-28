    AREA utils, CODE, READONLY
    
    ;FUNCTION START;===========================================================
	EXPORT getPSR
        ;Function that takes no argument and returns the PSR
	;--------------------------------------------------------------------------
getPSR
    MRS     r0, PSR
    BX      lr  
    ;FUNCTION END;=============================================================
	    
	
	;FUNCTION START;===========================================================
	EXPORT getCONTROL
        ;Function that takes no argument and returns the Control regsiters
	;--------------------------------------------------------------------------
getCONTROL
    MRS     r0, CONTROL
    BX      lr
	;FUNCTION END;=============================================================

    END