#include "cws_proxy.h"
#include "web_socket.h"
#include "up.pb.h"
#include "memory_pool.h"

CWs_Proxy::CWs_Proxy():
	fd_(-1),
	buff_(),
	buff_size_(0),
	hdl_(),
	pc_(),
	is_gate_(false),
	protobuf_up_(new Protobuf_Up)
{
}

CWs_Proxy::~CWs_Proxy(){
}

int CWs_Proxy::recv(){
	buff_size_ = read(fd_, buff_, BUFFLEN);
	return buff_size_;
}

int CWs_Proxy::handle_input(){
	DEBUG_LOG("fd:%d 返回消息至websocket", fd_);
	if(is_gate_){
		pc_->on_proxy_recv(hdl_, buff_, buff_size_, false);
		DEBUG_LOG("已返回%d字节", buff_size_);
	}
	else {
		Block_Buffer buf;
		buf.copy(buff_, buff_size_);
		process_block(buf);
	}
	return 0;
}

int CWs_Proxy::handle_output(std::string sInput){
	DEBUG_LOG("fd:%d 转发消息至proxy", fd_);
	size_t len;
	unsigned char * p = base64decode(sInput.c_str(),&len);
	int write_size =  write(fd_, (char *)p, len);
	fetch_substantial_info((char *)p);
	DEBUG_LOG("已发送%d字节", write_size);
	return write_size;
}

int CWs_Proxy::process_block(Block_Buffer& buf){
	uint16_t len = buf.read_uint16();
	uint32_t msg_id = buf.read_uint32();
	int32_t status = buf.read_int32();

	MSG_500000 msg;
	msg.deserialize(buf);
	DEBUG_LOG("len is %d, msg_id is %d, status is %d", msg_id, status);
	DEBUG_LOG("ip is %s, port is %d, session is %s", msg.ip.c_str(), msg.port, msg.session.c_str());
	if(status == 0){
		connect_to_gate();
		is_gate_ = true;
	}
	return 0;
}

bool CWs_Proxy::connect_to_login(){
	fd_ = socket(AF_INET, SOCK_STREAM, 0);
	connect(pc_->get_config().login_ip, pc_->get_config().login_port);
	MSG_100000 msg;
	msg.reset();
	std::string account = "alimi";
	std::string password = "testpassword";
	msg.account = account;
	msg.password = password;
	Block_Buffer buf;
	buf.make_client_message(1000, 1000, REQ_CLIENT_REGISTER, 0);
	msg.serialize(buf);
	buf.finish_message();
	write(fd_, (char *)buf.get_read_ptr(), buf.readable_bytes());
	return true;
}

bool CWs_Proxy::connect_to_gate(){
	if(fd_ != -1){
		NETWORK->degister_proxy(this, true);
	}
	fd_ = socket(AF_INET, SOCK_STREAM, 0);
	connect(pc_->get_config().proxy_ip, pc_->get_config().proxy_port);
	NETWORK->register_proxy(this);
	return true;
}

bool CWs_Proxy::connect(const char *ip, int port){
	struct sockaddr_in clientService;
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(ip);
	clientService.sin_port = htons(port);
	if(::connect(fd_, (struct sockaddr *)&clientService, sizeof(clientService)) < 0){
		DEBUG_LOG("连接服务器失败，错误代码：%d", errno);
		return false;
	}
	DEBUG_LOG("新连接建立！fd是%d", fd_);
	return true;
}

int CWs_Proxy::web_socket_close(){
	return pc_->web_socket_close(this);
}

int CWs_Proxy::on_proxy_close(){
	return pc_->on_proxy_close(hdl_);
}

std::string CWs_Proxy::encrypt(std::string plainText){
    std::string cipherText;
/*
    CryptoPP::AES::Encryption aesEncryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption( aesEncryption, iv );
    CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink( cipherText ));
    stfEncryptor.Put( reinterpret_cast<const unsigned char*>( plainText.c_str() ), plainText.length() + 1 );
    stfEncryptor.MessageEnd();

    string cipherTextHex;
    for( int i = 0; i < cipherText.size(); i++ )
    {
        char ch[3] = {0};
        sprintf(ch, "%02x",  static_cast<byte>(cipherText[i]));
        cipherTextHex += ch;
    }
*/
    return cipherText;
}

void CWs_Proxy::fetch_substantial_info(char *input){
	size_t total_len = 0;
	int cmdid = 0;
	size_t dynamic_len = 0;
	int8_t mode = 0;
	int16_t uin_len = 0;
	char *uin = NULL;
	char *aes_str = NULL;
	memcpy(&total_len, input, 4);
	memcpy(&cmdid, input + 4, 4);
	if(cmdid == 1){
		return;
	}
	memcpy(&dynamic_len, input + 8, 4);
	memcpy(&mode, input + 12, 1);
	memcpy(&uin_len, input + 13, 2);
	uin = (char *)MEMORY_POOL->get_memory(uin_len + 1);
	memset(uin, 0, uin_len + 1);
	memcpy(uin, input + 15, uin_len);
	size_t aes_len = total_len - uin_len - 15;
	aes_str = (char *)MEMORY_POOL->get_memory(aes_len + 1);
	memcpy(aes_str, input + 15 + uin_len, aes_len);
	DEBUG_LOG("SUBSTANTIAL INFO:\n"
				"\tDYNAMIC_LEN IS %d\n"
				"\tMODE IS %d\n"
				"\tUIN_LEN IS %d\n"
				"\tUIN IS %s",
				dynamic_len, mode, uin_len, uin);

	size_t protobuf_len = 0;
	memcpy(&protobuf_len, aes_str, 4);
	protobuf_up_->set_buf(aes_str + 4, aes_len);
	protobuf_up_->print();
	MEMORY_POOL->free_memory(uin);
	MEMORY_POOL->free_memory(aes_str);
}

