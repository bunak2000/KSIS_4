#define _WINSOCK_DEPRECATED_NO_WARNINGS


//#ifndef UNICODE
//#define UNICODE
//#endif

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include<iostream> 
#include<time.h> 

#pragma comment(lib, "Ws2_32.lib")


void LoadBlacklist(char** Blacklist)
{
	FILE* BlacklistFile;
	fopen_s(&BlacklistFile,"Blacklist.txt", "r+");
	*Blacklist = new char[20480];
	(*Blacklist)[0] = 0;
	char Temp[1024];

	while ( (fscanf_s(BlacklistFile, "%s", Temp, 1024)) != EOF) 
	{
		strcat_s((*Blacklist), 20480,Temp);
		strcat_s((*Blacklist), 20480, ",");
	}
	fclose(BlacklistFile);
}

int TryGetPort(char* URL) 
{
	char TempPort[10];
	int Length = strlen(URL);
	int j = 0;
	int Result = -1;
	TempPort[0] = 0;
	if (URL != NULL) 
	{
		for (int i = 0; i < Length; i++)
		{
			if ((URL[i] == ':') && ( i > 7 ))
			{
				i++;
				while ( (URL[i] != '/') && (i < strlen(URL)) )
				{
					TempPort[j] = URL[i];
					j++;
					i++;
				}
				TempPort[j] = 0;
				break;
			}
		}
		if (strlen(TempPort) > 0) 
		{
			Result = 0;
			for (int i = 0; i < strlen(TempPort); i++)
			{
				Result = Result * 10 + TempPort[i] - 48;
			}
		}	
	}
	return Result;
}

void ListenServer(SOCKET Browser, SOCKET SiteSocket, FILE* Log) 
{
	int RecivedBytes;
	char Data[10240];
	char TempLog[1024];

	while ((RecivedBytes = recv(SiteSocket, Data, 10240, 0)) != INVALID_SOCKET)
	{

		TempLog[0] = 0;
		
		if ((RecivedBytes > 3) && (Data[0] == 'H') && (Data[1] == 'T') && (Data[2] == 'T')) 
		{
			for (int i = 0; i < RecivedBytes; i++)
			{
				TempLog[i] = Data[i]; 

				if (Data[i] == '\n')
				{
					TempLog[i + 1] = 0;
					break;
				}
			}

			if ((Log != NULL) && (TempLog[0] != 0))
			{
				fprintf_s(Log, TempLog);
			}		
		}

		send(Browser, Data, RecivedBytes, 0);
	}
	return;
}

void ExtractURL(char Data[], int DataLength, char** URL) 
{
	char TempUrl[2048];
	for (int i = 0; i < DataLength; i++)
	{
		if (Data[i] == ' ') 
		{
			i++;
			int j = i;
			while (Data[j] != ' ')
			{
				TempUrl[j - i] = Data[j];
				j++;
			}
			TempUrl[j - i] = 0;
			puts(TempUrl);
			break;
		}
	}
	*URL = new char[strlen(TempUrl)+2];

	for (int i = 0; i < strlen(TempUrl); i++)
	{
		(*URL)[i] = TempUrl[i];
	}
	(*URL)[strlen(TempUrl)] = 0;
}

void ExtractHost(char* URL, char** Host) 
{
	int i = 0;
	int Length = strlen(URL);
	int Start = 0;

	while (i < Length - 1)
	{
		if ((URL[i] == '/') && (URL[i + 1] == '/'))
		{
			Start = i + 2;
			break;
		}
		i++;
	}

	i = Start;
	char TempHost[2048];

	i = Start;
	while (i < Length)
	{
		TempHost[i - Start] = URL[i];
		if ((URL[i] == '/') || (URL[i] == ':'))
		{
			break;
		}
		i++;
	}
	TempHost[i - Start] = 0;

	*Host = new char[strlen(TempHost)+2];	

	for (int i = 0; i < strlen(TempHost); i++)
	{
		(*Host)[i] = TempHost[i];
	}
	(*Host)[strlen(TempHost)] = 0;
}

void ProxyStream(SOCKET ProxySocket, char* Blacklist, FILE* Log)
{
	char Data[10240];
	int RecivedBytes;

	SOCKET SiteSocket;
	SiteSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (SiteSocket == INVALID_SOCKET)
	{
		printf("Socket error: \n");
		WSACleanup();
		return;
	}

	hostent* HostIP;

	RecivedBytes = recv(ProxySocket, Data, 10240, 0);

	char TempLog[1024];
	TempLog[0] = 0;
	for (int i = 0; i < RecivedBytes; i++)
	{
		TempLog[i] = Data[i];
		if (Data[i] == '\n') 
		{
			TempLog[i + 1] = 0;
			break;
		}
	}

	if ((Log != NULL) && (TempLog[0] != 0))
	{
		fprintf_s(Log, TempLog);
	}

	char* URL = NULL;
	ExtractURL(Data, RecivedBytes, &URL);

	if (strstr(Blacklist, URL) != NULL) 
	{
		send(ProxySocket, "HTTP/1.0 403 Forbidden\r\n", 24, 0);
		fprintf_s(Log, "HTTP/1.0 403 Forbidden\r\n");
		closesocket(ProxySocket);
		return;
	}
	
	char* Host = NULL;
	ExtractHost(URL,&Host);
	
	sockaddr_in SendToAdress;
	SendToAdress.sin_family = AF_INET;
	SendToAdress.sin_port = htons(80);

	int Port = TryGetPort(URL);
	if (Port != -1) 
	{
		SendToAdress.sin_port = htons(Port);
	}

	if (inet_addr(Host) != INADDR_NONE)
	{
		SendToAdress.sin_addr.s_addr = inet_addr(Host);
	}
	else
	{
		if (HostIP = gethostbyname(Host))
		{
			SendToAdress.sin_addr.s_addr = ((unsigned long**)HostIP->h_addr_list)[0][0];
		}
		else
		{
			closesocket(ProxySocket);
			WSACleanup();
			return;
		}
	}
	
	if (connect(SiteSocket, (sockaddr*)&SendToAdress, sizeof(SendToAdress))) 
	{
		printf("Connection error\n");
		closesocket(ProxySocket);
		WSACleanup();
		return;
	}

	int SymbolsRemaining = 3;
	int j = 0;
	while ((SymbolsRemaining > 0) && (j < RecivedBytes))
	{
		if (Data[j] == '/')
		{
			SymbolsRemaining--;
		}
		j++;
	}
	j--;

	int End = j;
	j = 0;
	int Start = 0;
	while ( (j < RecivedBytes) && (Data[j] != ' ') )
	{
		j++;
	}
	Start = j;
	
	for (int i = Start; i < End; i++)
	{
		Data[i] = ' ';
	}
		
	int n = send(SiteSocket,Data, RecivedBytes, 0);
	
	std::thread Listen(ListenServer, ProxySocket, SiteSocket, Log);
	Listen.detach();

	while ((RecivedBytes = recv(ProxySocket, Data, 3000, 0)) != INVALID_SOCKET)
	{

		for (int i = 0; i < RecivedBytes; i++)
		{
			TempLog[i] = Data[i];
			if (Data[i] == '\n')
			{
				TempLog[i + 1] = 0;
				break;
			}
		}
		if (Log != NULL)
		{
			fprintf_s(Log, TempLog);
		}

		n = send(SiteSocket, Data, RecivedBytes, 0);

		if (n == 0) 
		{
			break;
		}
	}
}

void ReadCommands(FILE* Log, SOCKET ListenSocket) 
{
	char Command[1024];
	while (strcmp(Command,"exit") != 0)
	{
		gets_s(Command,1023);
	}
	fclose(Log);
	Log = NULL;
	closesocket(ListenSocket);
}

int main()
{
	char TempInfo[1024];

	if (WSAStartup(0x0202, (WSADATA*)&TempInfo[0]))
	{
		printf("WSAStartup error \n");
		return -1;
	}

	char* Blacklist;
	LoadBlacklist(&Blacklist);

	SOCKET ListenSocket;
	ListenSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (ListenSocket == INVALID_SOCKET)
	{
		printf("Socket error: \n");
		WSACleanup();
		return -1;
	}

	int ProxyPort = 8080;
	printf("Enter proxy port>>> ");
	scanf_s("%d",&ProxyPort);

	sockaddr_in ReciveAddress;
	ReciveAddress.sin_family = AF_INET;
	ReciveAddress.sin_port = htons(ProxyPort);	//61100 - my proxy port
	ReciveAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (bind(ListenSocket, (sockaddr*)&ReciveAddress, sizeof(ReciveAddress)))
	{
		printf("Error bind\n");
		closesocket(ListenSocket);
		WSACleanup();
		return -1;
	}

	if (listen(ListenSocket, 0x100))
	{
		printf("Error listen\n");
		closesocket(ListenSocket);
		WSACleanup();
		return -1;
	}


	SOCKET UserSocket;
	sockaddr_in UserAddress;
	int UserAddressSize = sizeof(UserAddress);

	FILE* Log;
	fopen_s(&Log,"log.txt","a");

	std::thread CommandConsole(ReadCommands, Log, ListenSocket);
	CommandConsole.detach();

	UserSocket = accept(ListenSocket, (sockaddr*)&UserAddress, &UserAddressSize);
	while (UserSocket != INVALID_SOCKET)
	{
		std::thread Proxy(ProxyStream,UserSocket, Blacklist, Log);
		Proxy.detach();

		UserSocket = accept(ListenSocket, (sockaddr*)&UserAddress, &UserAddressSize);
	}

	WSACleanup();

	system("Pause");
}
