#include "x2filterwheel.h"


X2FilterWheel::X2FilterWheel(const char* pszDriverSelection,
				const int& nInstanceIndex,
				SerXInterface					* pSerX, 
				TheSkyXFacadeForDriversInterface	* pTheSkyX, 
				SleeperInterface					* pSleeper,
				BasicIniUtilInterface			* pIniUtil,
				LoggerInterface					* pLogger,
				MutexInterface					* pIOMutex,
				TickCountInterface				* pTickCount)
{
	m_nPrivateMulitInstanceIndex	= nInstanceIndex;
	m_pSerX							= pSerX;		
	m_pIniUtil						= pIniUtil;
	m_pIOMutex						= pIOMutex;

    m_bLinked = false;
    m_RDC.SetSerxPointer(pSerX);
    m_RDC.setSleeper(pSleeper);

}

X2FilterWheel::~X2FilterWheel()
{
	if (m_pSerX)
		delete m_pSerX;
	if (m_pIniUtil)
		delete m_pIniUtil;
	if (m_pIOMutex)
		delete m_pIOMutex;
}


int	X2FilterWheel::queryAbstraction(const char* pszName, void** ppVal)
{
	X2MutexLocker ml(GetMutex());

	*ppVal = NULL;

    if (!strcmp(pszName, SerialPortParams2Interface_Name))
        *ppVal = dynamic_cast<SerialPortParams2Interface*>(this);

    return SB_OK;
}




#pragma mark - LinkInterface

int	X2FilterWheel::establishLink(void)
{
    int nErr;
    char szPort[DRIVER_MAX_STRING];

    X2MutexLocker ml(GetMutex());
    // get serial port device name
    portNameOnToCharPtr(szPort,DRIVER_MAX_STRING);
    nErr = m_RDC.Connect(szPort);
    if(nErr)
        m_bLinked = false;
    else
        m_bLinked = true;

    return nErr;
}

int	X2FilterWheel::terminateLink(void)
{
    X2MutexLocker ml(GetMutex());
    m_RDC.Disconnect();
    m_bLinked = false;
    return SB_OK;
}

bool X2FilterWheel::isLinked(void) const
{
    X2FilterWheel* pMe = (X2FilterWheel*)this;
    X2MutexLocker ml(pMe->GetMutex());
    return pMe->m_bLinked;
}

bool X2FilterWheel::isEstablishLinkAbortable(void) const	{

    return false;
}


#pragma mark - AbstractDriverInfo

#define DISPLAY_NAME "X2 RDC Filter Wheel Plug In"
void	X2FilterWheel::driverInfoDetailedInfo(BasicStringInterface& str) const
{
	str = "X2 RDC Filter Wheel Plug In";
}

double	X2FilterWheel::driverInfoVersion(void) const
{
	return 1.0;
}

void X2FilterWheel::deviceInfoNameShort(BasicStringInterface& str) const
{
	str = "X2 RDC Filter Wheel";
}

void X2FilterWheel::deviceInfoNameLong(BasicStringInterface& str) const
{
	str = "X2 RDC Filter Wheel Plug In";

}

void X2FilterWheel::deviceInfoDetailedDescription(BasicStringInterface& str) const
{
	str = "X2 RDC Filter Wheel Plug In by Rodolphe Pineau";
	
}

void X2FilterWheel::deviceInfoFirmwareVersion(BasicStringInterface& str)
{
    if(m_bLinked) {
        X2MutexLocker ml(GetMutex());
        char cFirmware[SERIAL_BUFFER_SIZE];
        m_RDC.getFirmwareVersion(cFirmware, SERIAL_BUFFER_SIZE);
        str = cFirmware;
    }
    else
        str = "N/A";
}
void X2FilterWheel::deviceInfoModel(BasicStringInterface& str)				
{
    if(m_bLinked) {
        X2MutexLocker ml(GetMutex());
        str = "Shutter";
    }
    else
        str = "N/A";
}

#pragma mark - FilterWheelMoveToInterface

int	X2FilterWheel::filterCount(int& nCount)
{
    int nErr = SB_OK;
    X2MutexLocker ml(GetMutex());
    nErr = m_RDC.getFilterCount(nCount);
    if(nErr) {
        nErr = ERR_CMDFAILED;
    }
    return nErr;
}

int	X2FilterWheel::defaultFilterName(const int& nIndex, BasicStringInterface& strFilterNameOut)
{
	X2MutexLocker ml(GetMutex());
    if(nIndex == 0)
        strFilterNameOut = "Dark";
    else if (nIndex == 1)
        strFilterNameOut = "Clear";
    else
        strFilterNameOut = "";

    return SB_OK;
}

int	X2FilterWheel::startFilterWheelMoveTo(const int& nTargetPosition)
{
    int nErr = SB_OK;
    
    if(m_bLinked) {
        X2MutexLocker ml(GetMutex());
        nErr = m_RDC.moveToFilterIndex(nTargetPosition);
        if(nErr)
            nErr = ERR_CMDFAILED;
    }
    return nErr;
}

int	X2FilterWheel::isCompleteFilterWheelMoveTo(bool& bComplete) const
{
    int nErr = SB_OK;

    if(m_bLinked) {
        X2FilterWheel* pMe = (X2FilterWheel*)this;
        X2MutexLocker ml(pMe->GetMutex());
        nErr = pMe->m_RDC.isMoveToComplete(bComplete);
        if(nErr)
            nErr = ERR_CMDFAILED;
    }
    return nErr;
}

int	X2FilterWheel::endFilterWheelMoveTo(void)
{
	X2MutexLocker ml(GetMutex());
	return SB_OK;
}

int	X2FilterWheel::abortFilterWheelMoveTo(void)
{
	X2MutexLocker ml(GetMutex());
	return SB_OK;
}

#pragma mark -  SerialPortParams2Interface

void X2FilterWheel::portName(BasicStringInterface& str) const
{
    char szPortName[DRIVER_MAX_STRING];

    portNameOnToCharPtr(szPortName, DRIVER_MAX_STRING);

    str = szPortName;

}

void X2FilterWheel::setPortName(const char* szPort)
{
    if (m_pIniUtil)
        m_pIniUtil->writeString(PARENT_KEY, CHILD_KEY_PORTNAME, szPort);

}


void X2FilterWheel::portNameOnToCharPtr(char* pszPort, const int& nMaxSize) const
{
    if (NULL == pszPort)
        return;

    snprintf(pszPort, nMaxSize,DEF_PORT_NAME);

    if (m_pIniUtil)
        m_pIniUtil->readString(PARENT_KEY, CHILD_KEY_PORTNAME, pszPort, pszPort, nMaxSize);
    
}




