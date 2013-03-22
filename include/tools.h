#ifndef TEMP_TEST_MY_FLF
#define TEMP_TEST_MY_FLF

#include <zmq.h>
#include <string>
#include <string.h>        // for bzero
#include <iostream>
#include <assert.h>
#include <stdint.h>    // for socket

using namespace std;


#define REV_BUF_SIZE 128 

inline void my_msg_send(void *sc, const std::string &data) {

	if (data.size() == 0) return ;

	static char send_buf[REV_BUF_SIZE];
	int pos=0;
	int rc=0;

	while (pos+REV_BUF_SIZE < data.length()) {
		
    rc = zmq_send (sc, data.data()+pos, REV_BUF_SIZE, ZMQ_SNDMORE);
		assert(rc == REV_BUF_SIZE);
		pos += REV_BUF_SIZE;
	}

  rc = zmq_send (sc, data.data()+pos, data.length()-pos, 0);

}


inline void my_msg_recv(void *sb, std::string &data) {

	static char rev_buf[REV_BUF_SIZE];

	int rcvmore=true;
	size_t sz = sizeof (rcvmore);
	int rc=0;

	data="";
	while (rcvmore) {
		bzero(rev_buf,REV_BUF_SIZE);
		rc = zmq_recv(sb, rev_buf, REV_BUF_SIZE, 0);
		assert (rc != -1);
		// no '\0' while be recieved
		data = data + std::string(rev_buf,rc);
		rc = zmq_getsockopt (sb, ZMQ_RCVMORE, &rcvmore, &sz);
	}
}


class Random {
	private:
		uint32_t seed_;
	public:
		explicit Random(uint32_t s) : seed_(s & 0x7fffffffu) { }
		uint32_t Next() {
			static const uint32_t M = 2147483647L;   // 2^31-1
			static const uint64_t A = 16807;  // bits 14, 8, 7, 5, 2, 1, 0
			uint64_t product = seed_ * A;

			seed_ = static_cast<uint32_t>((product >> 31) + (product & M));
			if (seed_ > M) {
				seed_ -= M;
			}
			return seed_;
		}
		uint32_t Uniform(int n) { return Next() % n; }

		bool OneIn(int n) { return (Next() % n) == 0; }

		uint32_t Skewed(int max_log) {
			return Uniform(1 << Uniform(max_log + 1));
		}
};


class RandomGenerator {
	public:

		std::string data_;
		int pos_;
		Random rand;      


		RandomGenerator() :rand(1000) {
			Random rnd(301);
			std::string piece;
			while (data_.size() < 1048576) {
				RandomString(&rnd, 100, &piece);
				data_.append(piece);
			}
			pos_ = 0;
		}

		void RandomString(Random* rnd, int len, std::string* dst) {
			dst->resize(len);
			for (int i = 0; i < len; i++) {
				(*dst)[i] = static_cast<char>(' ' + rnd->Uniform(95));  
			}
		}	

		std::string Generate(int len) {
			if (pos_ + len > data_.size()) {
				pos_ = 0;
				assert(len < data_.size());
			}
			pos_ += len;
			return std::string(data_.data() + pos_ - len, len);
		}
};




#endif
