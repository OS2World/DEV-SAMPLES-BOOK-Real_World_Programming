/* --------------------------------------------------------------------
                        Thermometer Header File
                              Chapter 6

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define TS_SHOW_PERCENT  0x0001

BOOL  APIENTRY DrawThermometer (HPS,PRECTL,LONG,LONG,LONG,LONG);
BOOL  APIENTRY DrawTemperature (HPS,PRECTL,LONG,LONG,LONG);

