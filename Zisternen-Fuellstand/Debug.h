#ifndef DEBUG_H
#define DEBUG_H

////////////////////////////////////////////
// Hilfskonstrukt, zum Debuggen
#ifdef DEBUG
#define D_PRINT(a)      Serial.print(a)
#define D_PRINTLN(a)    Serial.println(a)
#else
#define D_PRINT(a)      
#define D_PRINTLN(a)    
#endif

#ifdef DEBUG_SENSOR
#define D_P_SEN(a)   D_PRINT(a)
#define D_P_SENLN(a) D_PRINTLN(a)
#else
#define D_P_SEN(a)
#define D_P_SENLN(a)
#endif

#ifdef DEBUG_EEPROM
#define D_P_EEP(a)   D_PRINT(a)
#define D_P_EEPLN(a) D_PRINTLN(a)
#else
#define D_P_EEP(a)
#define D_P_EEPLN(a)
#endif

#ifdef DEBUG_USER_INPUT
#define D_P_UI(a)   D_PRINT(a)
#define D_P_UILN(a) D_PRINTLN(a)
#else
#define D_P_UI(a)
#define D_P_UILN(a)
#endif

#ifdef DEBUG_ANZEIGE
#define D_P_ANZ(a)   D_PRINT(a)
#define D_P_ANZLN(a) D_PRINTLN(a)
#else
#define D_P_ANZ(a)
#define D_P_ANZLN(a)
#endif

#ifdef DEBUG_DREHGEBER
#define D_P_DREH(a)   D_PRINT(a)
#define D_P_DREHLN(a) D_PRINTLN(a)
#else
#define D_P_DREH(a)
#define D_P_DREHLN(a)
#endif

#endif // DEBUG_H

