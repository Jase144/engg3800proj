#ifndef PERIPH_64LEDMATRIX_H
#define PERIPH_64LEDMATRIX_H

    #define VAL_64LEDMATRIX_REDID_external 0
    #define VAL_64LEDMATRIX_GREENID_external 1
    #define VAL_64LEDMATRIX_BLUEID_external 2
    #define VAL_64LEDMATRIX_YELLOWID_external 3
    #define VAL_64LEDMATRIX_WHITEID_external 4

    void reg_64ledmatrix_init_external(void);

    void reg_64ledmatrix_senddata(uint8_t rgbMatrix);

    extern uint8_t transferComplete;
#endif
