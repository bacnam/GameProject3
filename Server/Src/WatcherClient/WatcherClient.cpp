﻿#include "stdafx.h"
#include "WatcherClient.h"
#include "../Message/Msg_Game.pb.h"
#include "../Message/Msg_RetCode.pb.h"
#include "../Message/Msg_ID.pb.h"
CWatcherClient::CWatcherClient(void)
{
	m_dwWatchSvrConnID	= 0;
	m_dwWatchIndex		= 0;
	m_dwLastHeartTime   = 0;
}

CWatcherClient::~CWatcherClient(void)
{
}

CWatcherClient* CWatcherClient::GetInstancePtr()
{
	static CWatcherClient _WatcherClient;

	return &_WatcherClient;
}

BOOL CWatcherClient::SetWatchIndex(UINT32 nIndex)
{
	m_dwWatchIndex = nIndex;

	return TRUE;
}

BOOL CWatcherClient::OnNewConnect(UINT32 nConnID)
{
	if (nConnID == m_dwWatchSvrConnID)
	{
		SendWatchHeartBeat();
	}

	return TRUE;
}

BOOL CWatcherClient::OnCloseConnect(UINT32 nConnID)
{
	if (nConnID == m_dwWatchSvrConnID)
	{
		m_dwWatchSvrConnID = 0;
	}

	return TRUE;
}

BOOL CWatcherClient::OnSecondTimer()
{
	if (m_dwWatchSvrConnID != 0 && m_dwLastHeartTime != 0)
	{
		INT64 nCurTick = CommonFunc::GetTickCount();

		if (nCurTick - m_dwLastHeartTime > 2000)
		{
			ServiceBase::GetInstancePtr()->CloseConnect(m_dwWatchSvrConnID);

			m_dwWatchSvrConnID = 0;

			m_dwLastHeartTime = 0;

			return TRUE;
		}
	}

	SendWatchHeartBeat();

	return TRUE;
}

BOOL CWatcherClient::DispatchPacket(NetPacket* pNetPacket)
{
	switch(pNetPacket->m_dwMsgID)
	{
			PROCESS_MESSAGE_ITEM(MSG_WATCH_HEART_BEAT_ACK, OnMsgWatchHeartBeatAck)
	}

	return FALSE;
}


BOOL CWatcherClient::ConnectToWatchServer()
{
	if (m_dwWatchSvrConnID != 0)
	{
		return TRUE;
	}
	UINT32 nWatchPort = CConfigFile::GetInstancePtr()->GetRealNetPort("watch_svr_port");
	ERROR_RETURN_FALSE(nWatchPort > 0);
	std::string strWatchIp = CConfigFile::GetInstancePtr()->GetStringValue("watch_svr_ip");
	CConnection* pConnection = ServiceBase::GetInstancePtr()->ConnectTo(strWatchIp, nWatchPort);
	ERROR_RETURN_FALSE(pConnection != NULL);
	m_dwWatchSvrConnID = pConnection->GetConnectionID();
	return TRUE;
}

BOOL CWatcherClient::SendWatchHeartBeat()
{
	if (m_dwWatchIndex == 0)
	{
		return TRUE;
	}

	if (m_dwWatchSvrConnID == 0)
	{
		ConnectToWatchServer();
		return TRUE;
	}

	static int nSecond = 15;
	nSecond++;
	if (nSecond < 5)
	{
		return TRUE;
	}

	nSecond -= 5;

	WatchHeartBeatReq Req;
	Req.set_data(m_dwWatchIndex);
	Req.set_processid(CommonFunc::GetCurProcessID());
	ServiceBase::GetInstancePtr()->SendMsgProtoBuf(m_dwWatchSvrConnID, MSG_WATCH_HEART_BEAT_REQ, 0, 0, Req);
	m_dwLastHeartTime = CommonFunc::GetTickCount();
	return TRUE;
}

BOOL CWatcherClient::OnMsgWatchHeartBeatAck(NetPacket* pNetPacket)
{
	WatchHeartBeatAck Ack;
	Ack.ParsePartialFromArray(pNetPacket->m_pDataBuffer->GetData(), pNetPacket->m_pDataBuffer->GetBodyLenth());

	m_dwLastHeartTime = 0;
	return TRUE;
}