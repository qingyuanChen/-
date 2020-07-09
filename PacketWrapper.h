#ifndef PACKETWRAPPER_H
#define PACKETWRAPPER_H
typedef unsigned char byte;

typedef enum{
	HJCOMGETFILE = 0,
	HJCOMGETDIRECTORY,
	HJCOMPOST,
	HJCOMFILE,
	HJCOMDIRECTORY
}HJCOMPACKETTYPE;

typedef struct{
	int id;
	byte p_type;
	int datalen;
	byte subpackageNum;
	byte subpackageId;

	CString ToString(){
		CString str;
		str.Format("id:%d,p_type:%d,datalen:%d,subpackageNum:%d,subpackageId:%d.", id, p_type, datalen, subpackageNum, subpackageId);
		return str;
	}
}HJCOMPACKETHEAD;

class PacketWrapper{
public:
	void getPacketHead(HJCOMPACKETTYPE tType, int datalen, void **dst, int *len);
	int getPacketFromData(void *src, HJCOMPACKETHEAD& dst);
};
#endif