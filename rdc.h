//
//  rdc.h
//  RDC FilterWheel
//
//  Created by Rodolphe Pineau on 10/5/2019.
//  Copyright © 2019 RTI-Zone. All rights reserved.
//

#ifndef rdc_h
#define rdc_h
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <time.h>
#ifdef SB_MAC_BUILD
#include <unistd.h>
#endif

#include <string>


#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"

// #define RDC_DEBUG 2


#define SERIAL_BUFFER_SIZE 32
#define MAX_TIMEOUT 1000            // in miliseconds
#define MAX_FILTER_CHANGE_TIMEOUT 25 // in seconds
#define LOG_BUFFER_SIZE 256

enum RDCFilterWheelErrors { RDC_OK=SB_OK, RDC_NOT_CONNECTED, RDC_CANT_CONNECT, RDC_BAD_CMD_RESPONSE, RDC_COMMAND_FAILED};
enum RDCSlots {CLOSED = 0, OPENED, MOVING};

class CRDC
{
public:
    CRDC();
    ~CRDC();

    int             Connect(const char *szPort);
    void            Disconnect(void);
    bool            IsConnected(void) { return m_bIsConnected; };

    void            SetSerxPointer(SerXInterface *p) { m_pSerx = p; };
    void            setSleeper(SleeperInterface *pSleeper) { m_pSleeper = pSleeper; };

    // filter wheel communication
    int             filterWheelCommand(const char *szCmd, char *szResult, int nResultMaxLen);
    int             readResponse(char *szRespBuffer, int nBufferLen);

    // Filter Wheel commands
    int             getFirmwareVersion(char *szVersion, int nStrMaxLen);

    int             moveToFilterIndex(int nTargetPosition);
    int             isMoveToComplete(bool &bComplete);

    int             getFilterCount(int &nCount);
    int             getCurrentSlot(int &nSlot);

protected:
    SerXInterface   *m_pSerx;
    SleeperInterface    *m_pSleeper;
    
    bool            m_bIsConnected;

    char            m_szFirmwareVersion[SERIAL_BUFFER_SIZE];

    char            m_szLogBuffer[LOG_BUFFER_SIZE];

    int             m_nCurentFilterSlot;
    int             m_nTargetFilterSlot;
    
    int             m_nNbSlot;

#ifdef RDC_DEBUG
    std::string m_sLogfilePath;
    // timestamp for logs
    char *timestamp;
    time_t ltime;
    FILE *Logfile;      // LogFile
#endif

};
#endif /* rdc_h */
