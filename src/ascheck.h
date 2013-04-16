#ifndef ASIAINFO_BILL_CHECK_69858c38
#define ASIAINFO_BILL_CHECK_69858c38



#include <string>

//#define AS_E_XXXXX  error code define 
#define AS_E_ENV_CONFIG_SERVER_UNDFINE -10
#define AS_E_ZMQ_SEND_DATA_ERROR -11
#define AS_E_ZMQ_RECV_DATA_ERROR -12
#define AS_E_CREATE_SOCKET_ERROR -13
#define AS_E_SOCKECT_BIND_ERROR -14
#define AS_E_IP_ADDRESS_ERROR -15
#define AS_E_CONNECT_TO_SERVER_ERROR -16
#define AS_E_ZMQ_CTX_NEW_ERROR -17
#define AS_E_ZMQ_SOCKET_ERROR -18
#define AS_E_ZMQ_RECV_DATA_IS_NULL -19
#define AS_E_TCP_SEND_DATA_ERROR -20
#define AS_E_TCP_RECV_DTA_ERROR -21
#define AS_E_ZMQ_CONNECT_ERROR -22

#define AS_E_ASCHECK_SERVER_NOT_IN_POOL -50
#define AS_E_PROTO_PRASE_ERROR -51
#define AS_E_SEND_DATA_IS_TO_LARGE -52
#define AS_E_CONFIG_ERROR -53

#define AS_E_UNKNOWN_ERROR -99


class ascheck {

public:

	/*
	 *  if key is in db , return 1
	 *  else inset key/value to db , and return 0;
	 *  err happen ,return <0 ,
	 */ 

	static int add(const std::string &part,const std::string &key,const std::string &value);

	/*
	 *  success return 0;
	 *  no this value return 1;
	 *  err happen ,return <0 ,
	 */ 

	static int query(const std::string &part,const std::string &key,std::string &value);

	/*
	 *  sucess return 0;
	 *  err happen ,return <0 ,
	 */ 

	static int del(const std::string &part,const std::string &key);

};


#endif
