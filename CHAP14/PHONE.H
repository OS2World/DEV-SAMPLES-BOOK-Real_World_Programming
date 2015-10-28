/* --------------------------------------------------------------------
                           Phone Header File
                              Chapter 14

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define FONTNAMELEN 7         /* length + 1 */
#define HELV8FONT   "8.Helv"


/* Dialog and Control Identifiers     */

#define ID_APPNAME                     1

#define IDD_SETTINGS                  10
#define IDD_STATUS                    20
#define IDD_BUTTONS                   30

#define IDB_LED                       50

#define IDC_PORT                     100
#define IDC_BAUDRATE                 101 
#define IDC_DATABITS_5               103
#define IDC_DATABITS_6               104
#define IDC_DATABITS_7               105
#define IDC_DATABITS_8               106
#define IDC_STOPBITS_1               107
#define IDC_STOPBITS_15              108
#define IDC_STOPBITS_2               109
#define IDC_PARITY_NONE              110
#define IDC_PARITY_ODD               111
#define IDC_PARITY_EVEN              112
#define IDC_PARITY_MARK              113
#define IDC_PARITY_SPACE             114
#define IDC_HANDSHAKING_DTRC         115
#define IDC_HANDSHAKING_DTRI         116
#define IDC_HANDSHAKING_CTSO         117
#define IDC_HANDSHAKING_DSRO         118
#define IDC_HANDSHAKING_DCDO         119
#define IDC_HANDSHAKING_DSRI         120
#define IDC_HANDSHAKING_RTSI         121
#define IDC_FLOWCONTROL_XONXOFFT     122
#define IDC_XONCHAR                  123
#define IDC_XOFFCHAR                 124
#define IDC_FLOWCONTROL_XONXOFFR     125
#define IDC_FLOWCONTROL_ERRORREP     126
#define IDC_FLOWCONTROL_NULLSTRIP    127
#define IDC_FLOWCONTROL_BREAKREP     128
#define IDC_FLOWCONTROL_RTSC         129
#define IDC_FLOWCONTROL_TOGGLING     130
#define IDC_ERRORREPCHAR             131
#define IDC_BREAKREPCHAR             132
#define IDC_TIMEOUT_WRITE            133
#define IDC_TIMEOUT_READ             134
                                      
#define IDC_COMMEVENT_CTS            200
#define IDC_COMMEVENT_DSR            201
#define IDC_COMMEVENT_DCD            202
#define IDC_COMMEVENT_BRK            203
#define IDC_COMMEVENT_ERR            204
#define IDC_COMMEVENT_RI             205
#define IDC_COMMSTATUS_CTS           206
#define IDC_COMMSTATUS_DST           207
#define IDC_COMMSTATUS_DCD           208
#define IDC_COMMSTATUS_XON           209
#define IDC_COMMSTATUS_XOF           210
#define IDC_COMMSTATUS_BRK           211
#define IDC_COMMSTATUS_IMM           212
#define IDC_COMMSTATUS_DSR           213
#define IDC_QUEUE_IN                 214
#define IDC_QUEUE_OUT                215
#define IDC_MODEM_CTS                216
#define IDC_MODEM_DSR                218
#define IDC_MODEM_RI                 219
#define IDC_MODEM_DCD                220
#define IDC_MODEM_DTR                221
#define IDC_MODEM_RTS                222
#define IDC_LINESTATUS_WRQ           223
#define IDC_LINESTATUS_DTQ           224
#define IDC_LINESTATUS_HTX           225
#define IDC_LINESTATUS_CRS           226
#define IDC_LINESTATUS_XON           227
#define IDC_LINESTATUS_XOF           228
#define IDC_LINESTATUS_RED           229
#define IDC_LINESTATUS_WRT           230
#define IDC_COMMERROR_QOV            231
#define IDC_COMMERROR_HOV            232
#define IDC_COMMERROR_PAR            233 
#define IDC_COMMERROR_FRM            234

#define IDC_CONNECT                  300
#define IDC_HANGUP                   301
#define IDC_SETTINGS                 302
#define IDC_TRANSMIT                 303
#define IDC_ABOUT                    304
                                     
