#ifndef __PROTOBUF_UP__
#define __PROTOBUF_UP__

#include "up.pb.h"

class Protobuf_Up {
public:
	Protobuf_Up();
	~Protobuf_Up();
	void set_buf(char *buf, size_t len);
	void print();
private:
private:
	up::up_msg *msg_;
};

#endif

