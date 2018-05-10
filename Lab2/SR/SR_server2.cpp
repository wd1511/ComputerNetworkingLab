#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h> 
#include <time.h> 
#include <WinSock2.h> 
#include <fstream> 

#pragma comment(lib,"ws2_32.lib") 

#define SERVER_PORT  12340  //�˿ں� 
#define SERVER_IP    "0.0.0.0"  //IP ��ַ 
const int BUFFER_LENGTH = 1026;    //��������С������̫���� UDP ������֡�а�����ӦС�� 1480 �ֽڣ� 
const int SEND_WIND_SIZE = 10;//���ʹ��ڴ�СΪ 10��GBN ��Ӧ����  W + 1 <= N��W Ϊ���ʹ��ڴ�С��N Ϊ���кŸ�����
							  //����ȡ���к� 0...19 �� 20 �� 
							  //��������ڴ�С��Ϊ 1����Ϊͣ-��Э�� 

const int SEQ_SIZE = 20; //���кŵĸ������� 0~19 ���� 20 �� 
						 //���ڷ������ݵ�һ���ֽ����ֵΪ 0�������ݻᷢ��ʧ��
						 //��˽��ն����к�Ϊ 1~20���뷢�Ͷ�һһ��Ӧ 

int ack[SEQ_SIZE];//�յ� ack �������Ӧ 0~19 �� ack 
int curSeq;//��ǰ���ݰ��� seq 
int curAck;//��ǰ�ȴ�ȷ�ϵ� ack 
int totalSeq;//�յ��İ������� 
int totalPacket;//��Ҫ���͵İ����� 
int totalAck = 0;//�Ѿ�ȷ�ϵİ�����

//������ֿ�ʼ��ʱ�������ã�����������޶���������С
int remainingPacket;//��ʣ���û�����ģ�ע����û�����������ľ��㶪ҲҲ�Ƿ�����

//************************************ 
// Method:        getCurTime 
// FullName:    getCurTime 
// Access:        public   
// Returns:      void 
// Qualifier:  ��ȡ��ǰϵͳʱ�䣬������� ptime �� 
// Parameter: char * ptime 
//************************************ 
void getCurTime(char *ptime) {
	char buffer[128];
	memset(buffer, 0, sizeof(buffer));
	time_t c_time;
	struct tm *p;
	time(&c_time);
	p = localtime(&c_time);
	sprintf_s(buffer, "%d/%d/%d %d:%d:%d",
		p->tm_year + 1900,
		p->tm_mon,
		p->tm_mday,
		p->tm_hour,
		p->tm_min,
		p->tm_sec);
	strcpy_s(ptime, sizeof(buffer), buffer);
}

//************************************ 
// Method:        seqIsAvailable 
// FullName:    seqIsAvailable 
// Access:        public   
// Returns:      bool 
// Qualifier:  ��ǰ���к�  curSeq  �Ƿ���� 
//************************************ 
int seqIsAvailable() {
	/*int step;
	step = curSeq - curAck;
	step = step >= 0 ? step : step + SEQ_SIZE;
	//���к��Ƿ��ڵ�ǰ���ʹ���֮�� 
	if (step >= SEND_WIND_SIZE) {
		return false;
	}
	if (ack[curSeq] == 1 || ack[curSeq] == 2) {
		return true;
	}*/
	for (int i = 0; i < SEND_WIND_SIZE && i<remainingPacket; ++i) {
		int index = (i + curAck) % SEQ_SIZE;
		if (ack[index] == 1 || ack[index] == 2) {
			return index;
		}
	}
	return -1;
}

//************************************ 
// Method:        timeoutHandler 
// FullName:    timeoutHandler 
// Access:        public   
// Returns:      void 
// Qualifier:  ��ʱ�ش��������������������ڵ�����֡��Ҫ�ش� 
//************************************ 
void timeoutHandler() {
	printf("Timer out error.\n");
	int index, number = 0;
	for (int i = 0; i< SEND_WIND_SIZE; ++i) {
		index = (i + curAck) % SEQ_SIZE;
		if (ack[index] == 0)
		{
			ack[index] = 2;
			number++;
		}
	}
	totalSeq = totalSeq - number;
	curSeq = curAck;
}

//************************************ 
// Method:        ackHandler 
// FullName:    ackHandler 
// Access:        public   
// Returns:      void 
// Qualifier:  �յ� ack���ۻ�ȷ�ϣ�ȡ����֡�ĵ�һ���ֽ� 
//���ڷ�������ʱ����һ���ֽڣ����кţ�Ϊ 0��ASCII��ʱ����ʧ�ܣ���˼�һ�ˣ��˴���Ҫ��һ��ԭ
// Parameter: char c 
//************************************ 
void ackHandler(char c) {
	unsigned char index = (unsigned char)c - 1; //���кż�һ ����ʾ�Է�ȷ���յ���index�Ű�
	printf("Recv a ack of %d\n", index);
	// �յ���ACK>=�ȴ���ACK �� �յ���ACK�ڴ����ڣ� ���ڿ����end->0����һ������ �� ������� ��
	if (curAck <= index && (curAck + SEND_WIND_SIZE >= SEQ_SIZE ? true : index<curAck + SEND_WIND_SIZE)) {

		ack[index] = 3;
		while (ack[curAck] == 3) {
			ack[curAck] = 1;
			curAck = (curAck + 1) % SEQ_SIZE;
			totalAck++;
			remainingPacket--;

		}
	}
}

//������ 
int main(int argc, char* argv[])
{
	//�����׽��ֿ⣨���룩 
	WORD wVersionRequested;
	WSADATA wsaData;
	//�׽��ּ���ʱ������ʾ 
	int err;
	//�汾 2.2 
	wVersionRequested = MAKEWORD(2, 2);
	//���� dll �ļ� Scoket ��   
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		//�Ҳ��� winsock.dll 
		printf("WSAStartup failed with error: %d\n", err);
		return -1;
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		printf("Could not find a usable version of Winsock.dll\n");
		WSACleanup();
	}
	else {
		printf("The Winsock 2.2 dll was found okay\n");
	}
	SOCKET sockServer = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	//�����׽���Ϊ������ģʽ 
	int iMode = 1; //1����������0������ 
	ioctlsocket(sockServer, FIONBIO, (u_long FAR*) &iMode);//���������� 
	SOCKADDR_IN addrServer;   //��������ַ 
							  //addrServer.sin_addr.S_un.S_addr = inet_addr(SERVER_IP); 
	addrServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//���߾��� 
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(SERVER_PORT);
	err = bind(sockServer, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
	if (err) {
		err = GetLastError();
		printf("Could  not  bind  the  port  %d  for  socket.Error  code is %d\n", SERVER_PORT, err);
		WSACleanup();
		return -1;
	}

	SOCKADDR_IN addrClient;   //�ͻ��˵�ַ 
	int length = sizeof(SOCKADDR);
	char buffer[BUFFER_LENGTH]; //���ݷ��ͽ��ջ����� 
	ZeroMemory(buffer, sizeof(buffer));
	//���������ݶ����ڴ� 

	HANDLE fhadle = CreateFile("../test.txt",
		0, 0, NULL, OPEN_ALWAYS, 0, 0
	);
	int length_lvxiya = GetFileSize(fhadle, 0);
	totalPacket = length_lvxiya / 1024 + 1;
	remainingPacket = totalPacket;//һ��ʼ����������һ����
	char *data = new char[1024 * (totalPacket + SEND_WIND_SIZE*SEND_WIND_SIZE)];
	ZeroMemory(data, 1024 * (totalPacket + SEND_WIND_SIZE * SEND_WIND_SIZE));
	std::ifstream icin;
	icin.open("../test.txt");

	//char data[1024 * 113];
	//ZeroMemory(data, sizeof(data));
	icin.read(data, 1024 * (totalPacket + SEND_WIND_SIZE * SEND_WIND_SIZE));
	icin.close();



	int recvSize;
	for (int i = 0; i < SEQ_SIZE; ++i) {
		ack[i] = 1;
	}
	while (true) {
		//���������գ���û���յ����ݣ�����ֵΪ-1 
		recvSize =
			recvfrom(sockServer, buffer, BUFFER_LENGTH, 0, ((SOCKADDR*)&addrClient), &length);
		if (recvSize < 0) {
			Sleep(200);
			continue;
		}
		printf("recv from client: %s\n", buffer);
		if (strcmp(buffer, "-time") == 0) {
			getCurTime(buffer);
		}
		else if (strcmp(buffer, "-quit") == 0) {
			strcpy_s(buffer, strlen("Good bye!") + 1, "Good bye!");
		}
		else if (strcmp(buffer, "-testgbn") == 0) {
			//���� gbn ���Խ׶� 
			//���� server��server ���� 0 ״̬���� client ���� 205 ״̬�루server���� 1 ״̬�� 
			//server  �ȴ� client �ظ� 200 ״̬�룬����յ���server ���� 2 ״̬����	��ʼ�����ļ���������ʱ�ȴ�ֱ����ʱ\
							//���ļ�����׶Σ�server ���ʹ��ڴ�С��Ϊ 
			ZeroMemory(buffer, sizeof(buffer));
			int recvSize;
			int waitCount = 0;
			printf("Begain to test GBN protocol,please don't abort the process\n");
			//������һ�����ֽ׶� 
			//���ȷ�������ͻ��˷���һ�� 205 ��С��״̬�루���Լ�����ģ���ʾ������׼�����ˣ����Է�������
			//�ͻ����յ� 205 ֮��ظ�һ�� 200 ��С��״̬�룬��ʾ�ͻ���׼�����ˣ����Խ���������
			//�������յ� 200 ״̬��֮�󣬾Ϳ�ʼʹ�� GBN ���������� 
			printf("Shake hands stage\n");
			int stage = 0;
			bool runFlag = true;
			int nowSeq;
			while (runFlag) {
				switch (stage) {
				case 0://���� 205 �׶� 
					buffer[0] = 205;
					sendto(sockServer, buffer, strlen(buffer) + 1, 0,
						(SOCKADDR*)&addrClient, sizeof(SOCKADDR));
					Sleep(100);
					stage = 1;
					break;
				case 1://�ȴ����� 200 �׶Σ�û���յ��������+1����ʱ������˴Ρ����ӡ����ȴ��ӵ�һ����ʼ
					recvSize = recvfrom(sockServer, buffer, BUFFER_LENGTH, 0, ((SOCKADDR*)&addrClient), &length);
					if (recvSize < 0) {
						++waitCount;
						if (waitCount > 20) {
							runFlag = false;
							printf("Timeout error\n");
							break;
						}
						Sleep(500);
						continue;
					}
					else {
						if ((unsigned char)buffer[0] == 200) {
							printf("Begin a file transfer\n");
							printf("File size is %dKB, each packet is 1024B and packet total num is %d\n", sizeof(data), totalPacket);
							curSeq = 0;
							curAck = 0;
							totalSeq = 0;
							waitCount = 0;
							stage = 2;
						}
					}
					break;
				case 2://���ݴ���׶� 
					nowSeq = seqIsAvailable();//��Ҫ���İ����
					if (nowSeq >= 0) {
						//���͸��ͻ��˵����кŴ� 1 ��ʼ 
						buffer[0] = nowSeq + 1;
						ack[nowSeq] = 0;
						//���ݷ��͵Ĺ�����Ӧ���ж��Ƿ������ 
						//Ϊ�򻯹��̴˴���δʵ�� 
						//memcpy(&buffer[1], data + 1024 * (curSeq + (totalSeq / SEND_WIND_SIZE)*SEND_WIND_SIZE), 1024);
						memcpy(&buffer[1], data + 1024 * (totalAck+nowSeq-curAck), 1024);
						printf("send a packet with a seq of %d\n", nowSeq);
						sendto(sockServer, buffer, BUFFER_LENGTH, 0,(SOCKADDR*)&addrClient, sizeof(SOCKADDR));
						//++curSeq;
						//curSeq %= SEQ_SIZE;
						//++totalSeq;
						Sleep(500);
					}
					//�ȴ� Ack����û���յ����򷵻�ֵΪ-1��������+1 
					recvSize = recvfrom(sockServer, buffer, BUFFER_LENGTH, 0, ((SOCKADDR*)&addrClient), &length);
					if (recvSize < 0) {
						waitCount++;
						//20 �εȴ� ack ��ʱ�ش� 
						if (waitCount > 20)
						{
							timeoutHandler();
							waitCount = 0;
						}
					}
					else {
						//�յ� ack 
						ackHandler(buffer[0]);
						if (totalAck == totalPacket) {
							stage = 3;//׼��������������
							waitCount = 21;//������һ�ξͿ���ֱ�ӷ�������Ϣ��ֻ����ʡ�¶���
						}
						waitCount = 0;
					}
					Sleep(500);
					break;
				case 3:
					memcpy(buffer, "ojbk\0", 5);
					if (waitCount > 20) {
						sendto(sockServer, buffer, strlen(buffer) + 1, 0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
						waitCount = 0;
					}
					else {
						recvSize = recvfrom(sockServer, buffer, BUFFER_LENGTH, 0, ((SOCKADDR*)&addrClient), &length);
						if (recvSize < 0) {
							waitCount++;
						}
						else {
							//�յ� ack 
							if (!memcmp(buffer, "otmspbbjbk\0", 11)) {
								printf("���ݴ���ɹ�������\n");
								runFlag = false;//�̳���Stage2�Ľ�����ʽ��û����֤�Ƿ���
							}
							//waitCount = 0;//�����ע�͵��ˣ���Ϊ������ֻ��Ҫ������ȷ����Ϣ������Ҫ���
						}
						break;
					}
					Sleep(500);
				}
			}
		}
		//�����ˣ�û�㶮����ΪʲôҪ����
		//sendto(sockServer, buffer, strlen(buffer) + 1, 0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
		Sleep(500);
	}
	//�ر��׽��֣�ж�ؿ� 
	closesocket(sockServer);
	WSACleanup();
	return 0;
}