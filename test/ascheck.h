#ifndef ASIAINFO_BILL_CHECK_69858c38
#define ASIAINFO_BILL_CHECK_69858c38



#include <string>

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
