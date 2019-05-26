//
//  xagyl.cpp
//  XagylFilterWheel
//
//  Created by Rodolphe Pineau on 26/10/16.
//  Copyright © 2016 RTI-Zone. All rights reserved.
//

#include "rdc.h"

CRDC::CRDC()
{
    m_bIsConnected = false;
    m_nCurentFilterSlot = -1;
    m_nTargetFilterSlot = 0;
    m_nNbSlot = 0;
    m_pSleeper = NULL;
    

#ifdef RDC_DEBUG
#if defined(SB_WIN_BUILD)
    m_sLogfilePath = getenv("HOMEDRIVE");
    m_sLogfilePath += getenv("HOMEPATH");
    m_sLogfilePath += "\\RDCLog.txt";
#elif defined(SB_LINUX_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/RDCLog.txt";
#elif defined(SB_MAC_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/RDCLog.txt";
#endif
    Logfile = fopen(m_sLogfilePath.c_str(), "w");
#endif
    
#if defined RDC_DEBUG && RDC_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CRDC::CRDC] New Constructor Called\n", timestamp);
    fflush(Logfile);
#endif
    
}

CRDC::~CRDC()
{

}

int CRDC::Connect(const char *szPort)
{
    int nErr = RDC_OK;

#if defined RDC_DEBUG && RDC_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CRDC::Connect] Called\n", timestamp);
    fflush(Logfile);
#endif

    // 9600 8N1
    if(m_pSerx->open(szPort, 9600, SerXInterface::B_NOPARITY, "-DTR_CONTROL 1") == 0)
        m_bIsConnected = true;
    else
        m_bIsConnected = false;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    // With DTR the arduino reset, let's wait for it to be back
    if(m_pSleeper)
        m_pSleeper->sleep(2000);
    
#if defined RDC_DEBUG && RDC_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [Connect::Connect] Connected.\n", timestamp);
    fflush(Logfile);
#endif

    
    // if any of this fails we're not properly connected or there is a hardware issue.
    nErr = getFirmwareVersion(m_szFirmwareVersion, SERIAL_BUFFER_SIZE);
    if(nErr) {
#if defined RDC_DEBUG && RDC_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [Connect::Connect] Error Getting Firmware : %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        m_bIsConnected = false;
        m_pSerx->close();
        return FIRMWARE_NOT_SUPPORTED;
    }

#if defined RDC_DEBUG && RDC_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [Connect::Connect] Connected.\n", timestamp);
    fflush(Logfile);
#endif

    nErr = getCurrentSlot(m_nCurentFilterSlot);
    
    return nErr;
}



void CRDC::Disconnect()
{
#if defined RDC_DEBUG && RDC_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CRDC::Disconnect] Called\n", timestamp);
    fflush(Logfile);
#endif

    if(m_bIsConnected) {
        m_pSerx->purgeTxRx();
        m_pSerx->close();
    }
    m_bIsConnected = false;
}


#pragma mark - communication functions
int CRDC::readResponse(char *szRespBuffer, int nBufferLen)
{
    int nErr = RDC_OK;
    unsigned long ulBytesRead = 0;
    unsigned long ulTotalBytesRead = 0;
    char *szBufPtr;
    
    memset(szRespBuffer, 0, (size_t) nBufferLen);
    szBufPtr = szRespBuffer;

    do {
        nErr = m_pSerx->readFile(szBufPtr, 1, ulBytesRead, MAX_TIMEOUT);
        if(nErr) {
#if defined RDC_DEBUG && RDC_DEBUG >= 2
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] [CRDC::readResponse] readFile error.\n", timestamp);
            fflush(Logfile);
#endif
            return nErr;
        }

#if defined RDC_DEBUG && RDC_DEBUG >= 3
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CRDC::readResponse] respBuffer = %s\n", timestamp, szRespBuffer);
        fflush(Logfile);
#endif

        if (ulBytesRead !=1) {// timeout
#if defined RDC_DEBUG && RDC_DEBUG >= 2
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] [CRDC::readResponse] readFile Timeout\n", timestamp);
            fflush(Logfile);
#endif
            nErr = RDC_BAD_CMD_RESPONSE;
            break;
        }
        ulTotalBytesRead += ulBytesRead;
#if defined RDC_DEBUG && RDC_DEBUG >= 3
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CRDC::readResponse] ulBytesRead = %lu\n", timestamp, ulBytesRead);
        fprintf(Logfile, "[%s] [CRDC::readResponse] ulTotalBytesRead = %lu\n", timestamp, ulTotalBytesRead);
        fflush(Logfile);
#endif

    } while (*szBufPtr++ != '#' && ulTotalBytesRead < (unsigned long)nBufferLen );

    if(ulTotalBytesRead>1)
        *(szBufPtr-1) = 0; //remove the '#'

    return nErr;
}


int CRDC::filterWheelCommand(const char *szCmd, char *szResult, int nResultMaxLen)
{
    int nErr = RDC_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    unsigned long  ulBytesWrite;

    m_pSerx->purgeTxRx();
    
#if defined RDC_DEBUG && RDC_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CRDC::filterWheelCommand] Sending %s\n", timestamp, szCmd);
    fflush(Logfile);
#endif

    nErr = m_pSerx->writeFile((void *)szCmd, strlen(szCmd), ulBytesWrite);
    m_pSerx->flushTx();

    if(nErr){
#if defined RDC_DEBUG && RDC_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CRDC::filterWheelCommand] writeFile error.\n", timestamp);
        fflush(Logfile);
#endif
        return nErr;
    }

    if(szResult) {
        // read response
#if defined RDC_DEBUG && RDC_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CRDC::filterWheelCommand] Getting response.\n", timestamp);
        fflush(Logfile);
#endif
        nErr = readResponse(szResp, nResultMaxLen);
        if(nErr){
#if defined RDC_DEBUG && RDC_DEBUG >= 2
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] [CRDC::filterWheelCommand] readResponse error : %d\n", timestamp, nErr);
            fflush(Logfile);
            return nErr;
#endif
        }
#if defined RDC_DEBUG && RDC_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CRDC::filterWheelCommand] response = %s\n", timestamp, szResp);
        fflush(Logfile);
#endif
        strncpy(szResult, szResp, nResultMaxLen);
    }
    return nErr;
    
}

#pragma mark - Filter Wheel info commands

int CRDC::getFirmwareVersion(char *szVersion, int nStrMaxLen)
{
    int nErr = 0;
    char szResp[SERIAL_BUFFER_SIZE];

#if defined RDC_DEBUG && RDC_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CRDC::getFirmwareVersion] Called\n", timestamp);
    fflush(Logfile);
#endif

    if(!m_bIsConnected)
        return RDC_NOT_CONNECTED;

    nErr = filterWheelCommand("getFirmware#", szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
#if defined RDC_DEBUG && RDC_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CRDC::getFirmwareVersion] Error Getting response from filterWheelCommand : %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }
    
    strncpy(szVersion, szResp, nStrMaxLen);

#if defined RDC_DEBUG && RDC_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CRDC::getFirmwareVersion] Firmware '%s'\n", timestamp, szVersion);
    fflush(Logfile);
#endif

    
    return nErr;
}


#pragma mark - Filter Wheel move commands

int CRDC::moveToFilterIndex(int nTargetPosition)
{
    int nErr = 0;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];
    
#if defined RDC_DEBUG && RDC_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CRDC::moveToFilterIndex] Called, movig to filter %d\n", timestamp, nTargetPosition);
    fprintf(Logfile, "[%s] [CRDC::moveToFilterIndex] m_nCurentFilterSlot = %d\n", timestamp, m_nCurentFilterSlot);
    fflush(Logfile);
#endif

    if(nTargetPosition == CLOSED) {// close
        snprintf(szCmd,SERIAL_BUFFER_SIZE, "close#");
    }
    else if (nTargetPosition == OPENED) {
        snprintf(szCmd,SERIAL_BUFFER_SIZE, "open#");
    }
    nErr = filterWheelCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
#if defined RDC_DEBUG && RDC_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CRDC::moveToFilterIndex] Error Getting response from filterWheelCommand : %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }
    m_nTargetFilterSlot = nTargetPosition;

#if defined RDC_DEBUG && RDC_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CRDC::moveToFilterIndex] m_nCurentFilterSlot = %d\n", timestamp, m_nCurentFilterSlot);
    fprintf(Logfile, "[%s] [CRDC::moveToFilterIndex] m_nTargetFilterSlot = %d\n", timestamp, m_nTargetFilterSlot);
    fflush(Logfile);
#endif

    return nErr;
}

int CRDC::isMoveToComplete(bool &bComplete)
{
    int nErr = RDC_OK;
    int nFilterSlot = MOVING;
    char szResp[SERIAL_BUFFER_SIZE];

    bComplete = false;

    if(m_nCurentFilterSlot == m_nTargetFilterSlot) {
        bComplete = true;
        return nErr;
    }

    
    nErr = filterWheelCommand("getState#", szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
#if defined RDC_DEBUG && RDC_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CRDC::isMoveToComplete] Error Getting response from filterWheelCommand : %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }

#if defined RDC_DEBUG && RDC_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CRDC::isMoveToComplete] m_nCurentFilterSlot = %d\n", timestamp, m_nCurentFilterSlot);
    fprintf(Logfile, "[%s] [CRDC::isMoveToComplete] m_nTargetFilterSlot = %d\n", timestamp, m_nTargetFilterSlot);
    fflush(Logfile);
#endif

    // are we still moving ?
    if(strstr(szResp,"Closing") || strstr(szResp,"Opening")) {
        return RDC_OK;
    }
    else if (strstr(szResp,"Closed")) {
        nFilterSlot = CLOSED;
    }
    else if (strstr(szResp,"Opened")) {
        nFilterSlot = OPENED;
    }

    if(nFilterSlot == m_nTargetFilterSlot) {
        bComplete = true;
        m_nCurentFilterSlot = nFilterSlot;
    }

#if defined RDC_DEBUG && RDC_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CRDC::isMoveToComplete] Done, nErr =  %d\n", timestamp, nErr);
    fflush(Logfile);
#endif

    return nErr;
}


#pragma mark - filters and device params functions
int CRDC::getFilterCount(int &nCount)
{
    nCount = 2;
    return RDC_OK;
}


int CRDC::getCurrentSlot(int &nSlot)
{
    int nErr = RDC_OK;
    char szResp[SERIAL_BUFFER_SIZE];

    nSlot = CLOSED;

#if defined RDC_DEBUG && RDC_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CRDC::getCurrentSlot] Called\n", timestamp);
    fflush(Logfile);
#endif

    nErr = filterWheelCommand("getState#", szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
#if defined RDC_DEBUG && RDC_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CRDC::getCurrentSlot] Error Getting response from filterWheelCommand : %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }
    if (strstr(szResp,"Closed")) {
        nSlot = CLOSED;
    }
    else if (strstr(szResp,"Opened")) {
        nSlot = OPENED;
    }

    m_nCurentFilterSlot = nSlot;

#if defined RDC_DEBUG && RDC_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CRDC::getCurrentSlot] m_nCurentFilterSlot = %d\n", timestamp, m_nCurentFilterSlot);
    fflush(Logfile);
#endif

    
    return nErr;
}
