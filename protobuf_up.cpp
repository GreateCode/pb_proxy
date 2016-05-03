#include <string.h>
#include "protobuf_up.h"
#include "debug_log.h"

Protobuf_Up::Protobuf_Up():
	msg_(new up::up_msg)
{
	msg_->Clear();
}

Protobuf_Up::~Protobuf_Up(){

}

void Protobuf_Up::print(){
	if(msg_->has__repeat()){
		DEBUG_LOG("repeat is %d", msg_->_repeat());
	}
	if(msg_->has__user_id()){
		DEBUG_LOG("user_id is %d", msg_->_user_id());
	}
	if(msg_->has__sdk_login()){
		DEBUG_LOG("sdklogin session_key is %s, plat_id is %d", msg_->_sdk_login()._session_key().c_str(), msg_->_sdk_login()._plat_id());
	}
}

void Protobuf_Up::set_buf(char *buf, size_t len){
	msg_->Clear();
	msg_->ParseFromArray(buf, len);
}

