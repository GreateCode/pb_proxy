/*
 * Block_Buffer.h
 *
 *  Created on: Dec 16,2015
 *      Author: zhangyalei
 */

/*
 +------------+----------------+-----------------+
 | head_bytes | readable bytes | writeable bytes |
 |            |   (CONTENT)    |                 |
 +------------+----------------+-----------------+
  |                        |                 		  |
 0  read_index(init_offset)  write_index     vector::size()

client message head:
	int32(cid);
	int16(len);
	int32(serial_cipher);
	int32(msg_time_cipher);
	int32(msg_id);
	int32(status);

gate to game,master,login,chat message head:
	int32(cid);
	int16(len);
	int32(msg_id);
	int32(status);
	int32(player_cid);

inner message head;
	int32(cid);
	int16(len);
	int32(msg_id);
	int32(status);
 */

#ifndef BLOCK_BUFFER_H_
#define BLOCK_BUFFER_H_

#include <stdint.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <endian.h>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <algorithm>
#include "debug_log.h"

#define BLOCK_LITTLE_ENDIAN

class Block_Buffer {
public:
	Block_Buffer()
	: max_use_times_(1),
	  use_times_(0),
	  init_size_(2048),
	  init_offset_(16),
	  read_index_(16),
	  write_index_(16),
	  buffer_(2064)
	{ }

	inline void reset(void);
	inline void swap(Block_Buffer &block);

	/// 当前缓冲内可读字节数
	inline size_t readable_bytes(void) const;
	/// 当前缓冲内可写字节数
	inline size_t writable_bytes(void) const;

	inline char * get_read_ptr(void);
	inline char * get_write_ptr(void);
	inline size_t get_buffer_size(void);

	inline int get_read_idx(void);
	inline void set_read_idx(int ridx);
	inline int get_write_idx(void);
	inline void set_write_idx(int widx);

	inline void copy(Block_Buffer *block);
	inline void copy(std::string const &str);
	inline void copy(char const *data, size_t len);
	inline void copy(void const *data, size_t len);

	inline void ensure_writable_bytes(size_t len);

	inline char *begin_write_ptr(void);
	inline const char *begin_write() const;

	inline void dump(void);
	inline void dump_inner(void);
	inline void debug(void);

	inline int8_t peek_int8();
	inline int16_t peek_int16();
	inline int32_t peek_int32();
	inline int64_t peek_int64();
	inline uint8_t peek_uint8();
	inline uint16_t peek_uint16();
	inline uint32_t peek_uint32();
	inline uint64_t peek_uint64();
	inline double peek_double();
	inline bool peek_bool();
	inline std::string peek_string();

	inline int8_t read_int8();
	inline int16_t read_int16();
	inline int32_t read_int32();
	inline int64_t read_int64();
	inline uint8_t read_uint8();
	inline uint16_t read_uint16();
	inline uint32_t read_uint32();
	inline uint64_t read_uint64();
	inline double read_double();
	inline bool read_bool();
	inline std::string read_string();

	inline int write_int8(int8_t v);
	inline int write_int16(int16_t v);
	inline int write_int32(int32_t v);
	inline int write_int64(int64_t v);
	inline int write_uint8(uint8_t v);
	inline int write_uint16(uint16_t v);
	inline int write_uint32(uint32_t v);
	inline int write_uint64(uint64_t);
	inline int write_double(double v);
	inline int write_bool(bool v);
	inline int write_string(const std::string &str);

	//服务器内部，发送到db,log使用的消息
	inline void make_inner_message(int msg_id, int status = 0);
	//gate与game,master,chat,login转发到客户端的消息
	inline void make_player_message(int msg_id, int status, int player_cid);
	//客户端发到服务器的消息
	inline void make_client_message(int serial_cipher, int msg_time_cipher, int msg_id, int status);
	//完成消息的生成
	inline void finish_message(void);

	inline int move_data(size_t dest, size_t begin, size_t end);

	inline int insert_head(Block_Buffer *buf);

	inline size_t capacity(void);

	inline void recycle_space(void);

	inline bool is_legal(void);

	inline bool verify_read(size_t s);

	inline void log_binary_data(size_t len);

private:
	inline char *begin(void);
	inline const char *begin(void) const;
	inline void make_space(size_t len);

private:
	unsigned short max_use_times_;
	unsigned short use_times_;
	size_t init_size_;
	size_t init_offset_;
	size_t read_index_, write_index_;
	std::vector<char> buffer_;
};

////////////////////////////////////////////////////////////////////////////////
void Block_Buffer::reset(void) {
	++use_times_;
	recycle_space();

	ensure_writable_bytes(init_offset_);
	read_index_ = write_index_ = init_offset_;
}

void Block_Buffer::swap(Block_Buffer &block) {
	std::swap(this->max_use_times_, block.max_use_times_);
	std::swap(this->use_times_, block.use_times_);
	std::swap(this->init_size_, block.init_size_);
	std::swap(this->init_offset_, block.init_offset_);
	std::swap(this->read_index_, block.read_index_);
	std::swap(this->write_index_, block.write_index_);
	buffer_.swap(block.buffer_);
}

size_t Block_Buffer::readable_bytes(void) const {
	return write_index_ - read_index_;
}

size_t Block_Buffer::writable_bytes(void) const {
	return buffer_.size() - write_index_;
}

char *Block_Buffer::get_read_ptr(void) {
	return begin() + read_index_;
}

char *Block_Buffer::get_write_ptr(void) {
	return begin() + write_index_;
}

size_t Block_Buffer::get_buffer_size(void) {
	return buffer_.size();
}

int Block_Buffer::get_read_idx(void) {
	return read_index_;
}

void Block_Buffer::set_read_idx(int ridx) {
	if ((size_t)ridx > buffer_.size() || (size_t)ridx > write_index_) {
		DEBUG_LOG("set_read_idx error ridx = %d.", ridx);
		debug();
	}

	read_index_ = ridx;
}

int Block_Buffer::get_write_idx(void) {
	return write_index_;
}

void Block_Buffer::set_write_idx(int widx) {
	if ((size_t)widx > buffer_.size() || (size_t)widx < read_index_) {
		DEBUG_LOG("set_write_idx error widx = %d.", widx);
		debug();
	}

	write_index_ = widx;
}

void Block_Buffer::ensure_writable_bytes(size_t len) {
	if (writable_bytes() < len) {
		make_space(len);
	}
}

void Block_Buffer::make_space(size_t len) {
	int cond_pos = read_index_ - init_offset_;
	size_t read_begin, head_size;
	if (cond_pos < 0) {
		read_begin = init_offset_ = read_index_;
		head_size = 0;
		DEBUG_LOG("read_index_ = %u, init_offset_ = %u", read_index_, init_offset_);
	} else {
		read_begin = init_offset_;
		head_size = cond_pos;
	}

	if (writable_bytes() + head_size < len) {
		buffer_.resize(write_index_ + len);
	} else {
		/// 把数据移到头部，为写腾出空间
		size_t readable = readable_bytes();
		std::copy(begin() + read_index_, begin() + write_index_, begin() + read_begin);
		read_index_ = read_begin;
		write_index_ = read_index_ + readable;
	}
}

char *Block_Buffer::begin_write_ptr(void) {
	return begin() + write_index_;
}

const char *Block_Buffer::begin_write(void) const {
	return begin() + write_index_;
}

char *Block_Buffer::begin(void) {
	return &*buffer_.begin();
}

const char *Block_Buffer::begin(void) const {
	return &*buffer_.begin();
}

void Block_Buffer::copy(void const *data, size_t len) {
	copy(static_cast<const char*> (data), len);
}

void Block_Buffer::copy(char const *data, size_t len) {
	ensure_writable_bytes(len);
	std::copy(data, data + len, get_write_ptr());
	write_index_ += len;
}

void Block_Buffer::copy(std::string const &str) {
	copy(str.data(), str.length());
}

void Block_Buffer::copy(Block_Buffer *block) {
	copy(block->get_read_ptr(), block->readable_bytes());
}

void Block_Buffer::dump_inner(void) {
	int ridx = get_read_idx();
	uint16_t len = read_uint16();
	printf("len = %d\n", len);
	dump();
	set_read_idx(ridx);
}

void Block_Buffer::dump(void) {
	write(STDOUT_FILENO, this->get_read_ptr(), this->readable_bytes());
}

void Block_Buffer::debug(void) {
	DEBUG_LOG("read_index = %ul, write_index = %ul, buffer_.size = %ul.", read_index_, write_index_, buffer_.size());;
}

int8_t Block_Buffer::peek_int8() {
	int8_t v = 0;
	if (verify_read(sizeof(v))) {
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
	} else {
		DEBUG_LOG("out of range");
		return -1;
	}
	return v;
}

int16_t Block_Buffer::peek_int16() {
	int16_t v = 0;
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint16_t t, u;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		u = be16toh(t);
		memcpy(&v, &u, sizeof(v));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
#endif
	} else {
		DEBUG_LOG("out of range");
		return -1;
	}
	return v;
}

int32_t Block_Buffer::peek_int32() {
	int32_t v = 0;
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint32_t t, u;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		u = be32toh(t);
		memcpy(&v, &u, sizeof(v));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
#endif
	} else {
		DEBUG_LOG("out of range");
		return -1;
	}
	return v;
}

int64_t Block_Buffer::peek_int64() {
	int64_t v = 0;
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint64_t t, u;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		u = be64toh(t);
		memcpy(&v, &u, sizeof(v));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
#endif
	} else {
		DEBUG_LOG("out of range");
		return -1;
	}
	return v;
}

uint8_t Block_Buffer::peek_uint8() {
	uint8_t v = 0;
	if (verify_read(sizeof(v))) {
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
	} else {
		DEBUG_LOG("out of range");
		return -1;
	}
	return v;
}

uint16_t Block_Buffer::peek_uint16() {
	uint16_t v = 0;
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint16_t t;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		v = be16toh(t);
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
#endif
	} else {
		DEBUG_LOG("out of range");
		return -1;
	}
	return v;
}

uint32_t Block_Buffer::peek_uint32() {
	uint32_t v = 0;
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint32_t t;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		v = be32toh(t);
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
#endif
	} else {
		DEBUG_LOG("out of range");
		return -1;
	}
	return v;
}

uint64_t Block_Buffer::peek_uint64() {
	uint64_t v = 0;
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint64_t t, u;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		u = be64toh(t);
		memcpy(&v, &u, sizeof(v));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
#endif
	} else {
		DEBUG_LOG("out of range");
		return -1;
	}
	return v;
}

double Block_Buffer::peek_double() {
	double v = 0;
#ifdef BLOCK_BIG_ENDIAN
	uint64_t t, u;
	memcpy(&t, &(buffer_[read_index_]), sizeof(t));
	u = be64toh(t);
	memcpy(&v, &u, sizeof(v));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
	memcpy(&v, &(buffer_[read_index_]), sizeof(v));
#endif
	return v;
}

bool Block_Buffer::peek_bool() {
	bool v = false;
	if (verify_read(sizeof(v))) {
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
	} else {
		DEBUG_LOG("out of range");
		return -1;
	}
	return v;
}

std::string Block_Buffer::peek_string() {
	std::string str = "";
	uint16_t len = read_uint16();
	if (len < 0) return str;
	str.append(buffer_[read_index_], len);
	return str;
}

int8_t Block_Buffer::read_int8() {
	int8_t v = 0;
	if (verify_read(sizeof(v))) {
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
	} else {
		DEBUG_LOG("out of range");
		return -1;
	}
	return v;
}

int16_t Block_Buffer::read_int16() {
	int16_t v = 0;
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint16_t t, u;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		u = be16toh(t);
		memcpy(&v, &u, sizeof(v));
		read_index_ += sizeof(v);
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
#endif
	} else {
		DEBUG_LOG("out of range");
		return -1;
	}
	return v;
}

int32_t Block_Buffer::read_int32() {
	int32_t v = 0;
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint32_t t, u;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		u = be32toh(t);
		memcpy(&v, &u, sizeof(v));
		read_index_ += sizeof(v);
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
#endif
	} else {
		DEBUG_LOG("out of range");
		return -1;
	}
	return v;
}

int64_t Block_Buffer::read_int64() {
	int64_t v = 0;
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint64_t t, u;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		u = be64toh(t);
		memcpy(&v, &u, sizeof(v));
		read_index_ += sizeof(v);
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
#endif
	} else {
		DEBUG_LOG("out of range");
		return -1;
	}
	return v;
}

uint8_t Block_Buffer::read_uint8() {
	uint8_t v = 0;
	if (verify_read(sizeof(v))) {
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
	} else {
		DEBUG_LOG("out of range");
		return -1;
	}
	return v;
}

uint16_t Block_Buffer::read_uint16() {
	uint16_t v = 0;
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint16_t t;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		v = be16toh(t);
		read_index_ += sizeof(v);
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
#endif
	} else {
		DEBUG_LOG("out of range");
		return -1;
	}
	return v;
}

uint32_t Block_Buffer::read_uint32() {
	uint32_t v = 0;
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint32_t t;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		v = be32toh(t);
		read_index_ += sizeof(v);
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
#endif
	} else {
		DEBUG_LOG("out of range");
		return -1;
	}
	return v;
}

uint64_t Block_Buffer::read_uint64() {
	uint64_t v = 0;
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint64_t t;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		v = be64toh(t);
		read_index_ += sizeof(v);
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
#endif
	} else {
		DEBUG_LOG("out of range");
		return -1;
	}
	return v;
}

double Block_Buffer::read_double() {
	double v = 0;
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint64_t t, u;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		u = be64toh(t);
		memcpy(&v, &u, sizeof(v));
		read_index_ += sizeof(t);
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
#endif
	} else {
		DEBUG_LOG("out of range");
		return -1;
	}
	return v;
}

bool Block_Buffer::read_bool() {
	bool v = false;
	if (verify_read(sizeof(v))) {
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
	} else {
		DEBUG_LOG("out of range");
		return -1;
	}
	return v;
}

std::string Block_Buffer::read_string() {
	std::string str = "";
	uint16_t len = read_uint16();
	if (len < 0) return str;
	if (!verify_read(len)) return str;
	str.resize(len);
	memcpy((char *)str.c_str(), this->get_read_ptr(), len);
	read_index_ += len;
	return str;
}

int Block_Buffer::write_int8(int8_t v) {
	copy(&v, sizeof(v));
	return 0;
}

int Block_Buffer::write_int16(int16_t v) {
#ifdef BLOCK_BIG_ENDIAN
	uint16_t t, u;
	t = *((uint16_t *)&v);
	u = htobe16(t);
	copy(&u, sizeof(u));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
	copy(&v, sizeof(v));
#endif
	return 0;
}

int Block_Buffer::write_int32(int32_t v) {
#ifdef BLOCK_BIG_ENDIAN
	uint32_t t, u;
	t = *((uint32_t *)&v);
	u = htobe32(t);
	copy(&u, sizeof(u));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
	copy(&v, sizeof(v));
#endif
	return 0;
}

int Block_Buffer::write_int64(int64_t v) {
#ifdef BLOCK_BIG_ENDIAN
	uint64_t t, u;
	t = *((uint64_t *)&v);
	u = htobe64(t);
	copy(&u, sizeof(u));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
	copy(&v, sizeof(v));
#endif
	return 0;
}

int Block_Buffer::write_uint8(uint8_t v) {
	copy(&v, sizeof(v));
	return 0;
}

int Block_Buffer::write_uint16(uint16_t v) {
#ifdef BLOCK_BIG_ENDIAN
	uint16_t t;
	t = htobe16(v);
	copy(&t, sizeof(t));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
	copy(&v, sizeof(v));
#endif
	return 0;
}

int Block_Buffer::write_uint32(uint32_t v) {
#ifdef BLOCK_BIG_ENDIAN
	uint32_t t;
	t = htobe32(v);
	copy(&t, sizeof(t));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
	copy(&v, sizeof(v));
#endif
	return 0;
}

int Block_Buffer::write_uint64(uint64_t v) {
#ifdef BLOCK_BIG_ENDIAN
	uint64_t t;
	t = htobe64(v);
	copy(&t, sizeof(t));
#endif

#ifdef BLOCK_LITTLE_ENDIAN
	copy(&v, sizeof(v));
#endif
	return 0;
}

int Block_Buffer::write_double(double v) {
#ifdef BLOCK_BIG_ENDIAN
	uint64_t t, u;
	t = *((uint64_t *)&v);
	u = htobe64(t);
	copy(&u, sizeof(u));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
	copy(&v, sizeof(v));
#endif
	return 0;
}

int Block_Buffer::write_bool(bool v) {
	copy(&v, sizeof(v));
	return 0;
}

int Block_Buffer::write_string(const std::string &str) {
	uint16_t len = str.length();
	write_uint16(len);
	copy(str.c_str(), str.length());
	return 0;
}

void Block_Buffer::make_inner_message(int msg_id, int status) {
	write_int16(0); /// length
	write_int32(msg_id);
	write_int32(status);
}

void Block_Buffer::make_player_message(int msg_id, int status, int player_cid) {
	write_int16(0); /// length
	write_int32(msg_id);
	write_int32(status);
	write_int32(player_cid);
}

void Block_Buffer::make_client_message(int serial_cipher, int msg_time_cipher, int msg_id, int status) {
	write_int16(0); /// length
	write_int32(serial_cipher);
	write_int32(msg_time_cipher);
	write_int32(msg_id);
	write_int32(status);
}

void Block_Buffer::finish_message(void) {
	int len = readable_bytes() - sizeof(uint16_t);
	int wr_idx = get_write_idx();
	set_write_idx(get_read_idx());
	write_uint16(len);
	set_write_idx(wr_idx);
}

int Block_Buffer::move_data(size_t dest, size_t begin, size_t end) {
	if (begin >= end) {
		DEBUG_LOG("begin = %ul, end = %ul, dest = %ul.", begin, end, dest);
		return -1;
	}
	size_t len = end - begin;
	this->ensure_writable_bytes(dest + len);
	std::memmove(this->begin() + dest, this->begin() + begin, len);
	return 0;
}

int Block_Buffer::insert_head(Block_Buffer *buf) {
	if (! buf) {
		DEBUG_LOG("buf == 0");
		return -1;
	}

	size_t len = 0;
	if ((len = buf->readable_bytes()) <= 0) {
		return -1;
	}

	size_t dest = read_index_ + len;
	move_data(dest, read_index_, write_index_);
	std::memcpy(this->get_read_ptr(),  buf->get_read_ptr(), len);

	this->set_write_idx(this->get_write_idx() + len);
	return 0;
}

size_t Block_Buffer::capacity(void) {
	return buffer_.capacity();
}

void Block_Buffer::recycle_space(void) {
	if (max_use_times_ == 0)
		return;
	if (use_times_ >= max_use_times_) {
			std::vector<char> buffer_free(init_offset_ + init_size_);
 			buffer_.swap(buffer_free);
 			ensure_writable_bytes(init_offset_);
 			read_index_ = write_index_ = init_offset_;
			use_times_ = 0;
	}
}

bool Block_Buffer::is_legal(void) {
	return read_index_ < write_index_;
}

bool Block_Buffer::verify_read(size_t s) {
	return (read_index_ + s <= write_index_) && (write_index_ <= buffer_.size());
}

void Block_Buffer::log_binary_data(size_t len) {
	size_t real_len = (len > readable_bytes()) ? readable_bytes() : len;
	size_t end_index = read_index_ + real_len;
	std::stringstream str_stream;
	char str_buf[32];
	for (size_t i = read_index_; i < end_index; ++i) {
		memset(str_buf, 0, sizeof(str_buf));
		snprintf(str_buf, sizeof(str_buf), "0x%02x", (uint8_t)buffer_[i]);
		str_stream << str_buf << " ";
	}
	DEBUG_LOG("log_binary_data:[%s]", str_stream.str().c_str());
}

#endif /* BLOCK_BUFFER_H_ */
