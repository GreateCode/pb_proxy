#include "cws_proxy.h"
#include "web_socket.h"
#include "up.pb.h"
#include "memory_pool.h"

CWs_Proxy::CWs_Proxy():
	fd_(socket(AF_INET, SOCK_STREAM, 0)),
	buff_(),
	buff_size_(0),
	hdl_(),
	pc_(),
	protobuf_up_(new Protobuf_Up)
{
}

CWs_Proxy::~CWs_Proxy(){
}

int CWs_Proxy::recv(){
	buff_size_ = read(fd_, buff_, BUFFLEN);
	return buff_size_;
}

char *parse_to_hex(char *strhex, char *buff_, int buff_size_){
	for(int i = 0; i < buff_size_; i++){
		sprintf(strhex + i * 5, "0x%02x ", (unsigned char)buff_[i]);
	}
	return strhex;
}

int CWs_Proxy::handle_input(){
	//char strhex[65535 * 5] = {};
	DEBUG_LOG("fd:%d 返回消息至websocket", fd_);
	//DEBUG_LOG("字节流：%s", parse_to_hex(strhex, buff_, buff_size_));
	pc_->on_proxy_recv(hdl_, buff_, buff_size_, false);
	DEBUG_LOG("已返回%d字节", buff_size_);
	return 0;
}

int CWs_Proxy::handle_output(std::string sInput){
	char *strhex = NULL;
	DEBUG_LOG("fd:%d 转发消息至proxy", fd_);
	size_t len;
	unsigned char * p = base64decode(sInput.c_str(),&len);
	int write_size =  write(fd_, (char *)p, len);
	fetch_substantial_info((char *)p);
	//int write_size =  write(fd_, sInput.c_str(), sInput.length());
	strhex = (char *)MEMORY_POOL->get_memory(len * 5);
	DEBUG_LOG("发送字节流：%s", parse_to_hex(strhex, (char *)p, len));
	MEMORY_POOL->free_memory(strhex);
	DEBUG_LOG("已发送%d字节", write_size);
	return write_size;
}
	
bool CWs_Proxy::connect(const char *ip, int port){
	struct sockaddr_in clientService;
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(ip);
	clientService.sin_port = htons(port);
	if(::connect(fd_, (struct sockaddr *)&clientService, sizeof(clientService)) < 0){
		DEBUG_LOG("连接proxy服务器失败，错误代码：%d", errno);
		return false;
	}
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

