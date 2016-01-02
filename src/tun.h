#ifndef TUN_H_
#define TUN_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h> 
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>

#include "wspace_asym_util.h"

/* buffer for reading from tun/tap interface, must be >= 1500 */
//#define PKT_SIZE 2000   
#define CLIENT 0
#define SERVER 1
#define INVALID_MODE -1
#define UDP 0
#define TCP 1
#define RAW 2
#define PORT_ETH 55554
#define PORT_ATH 55555
#define BROADCAST_PORT_ATH 55555


// For raw sockets
#define ETH_HEADER_SIZE 14
#define IP_HEADER_SIZE  20
#define UDP_HEADER_SIZE 8
#define OFFSET_SIZE  (ETH_HEADER_SIZE+IP_HEADER_SIZE+UDP_HEADER_SIZE)
#define DEVNAME_SIZE 50
#define RAW_PKT_SIZE (1500+ETH_HEADER_SIZE)

#define MAX_RADIO 3

#define DIFS_80211b 28
#define DIFS_80211ag 28
#define SLOT_TIME 9




class RadioContext
{
public:
  	char server_ip_ath_[16];
	char broadcast_ip_ath_[16];
	struct sockaddr_in server_addr_ath_, client_addr_ath_, remote_;
	uint16 port_ath_, broadcast_port_ath_;
	int sock_fd_ath_;
	int net_fd_ath_;
	int net_wr_;
	int rate_;
	char mac_addr_ath_[18];
	int channel_;
	int chanbw_;
	RadioContext()
	{
	}
	~RadioContext()
	{

	}

};

class Tun
{
public:
	enum IOType
	{
		kTun=1,
		kNet=2,
	};

	Tun(): tun_type_(IFF_TUN), comm_mode_(UDP), radio_count_(0),radio_mode_(0),
		port_eth_(PORT_ETH), server_client_mod_(INVALID_MODE)
	{
		bzero(if_name_, sizeof(if_name_));
		bzero(dev_, sizeof(dev_));
		bzero(server_ip_eth_, sizeof(server_ip_eth_));
		bzero(ath_, sizeof(ath_));
		flow_control_ = false;
		for(int i = 0; i < MAX_RADIO; i++)
		{
			ath_[i].port_ath_ = PORT_ATH + i;
			ath_[i].broadcast_port_ath_ = BROADCAST_PORT_ATH + i;
		}
	}

	~Tun()
	{
		close(tun_fd_);
		close(sock_fd_eth_);
		close(net_fd_eth_);
		for(int i = 0; i < radio_count_; i++)
		{
			close(ath_[i].sock_fd_ath_);
			close(ath_[i].net_fd_ath_);
		}
		if (comm_mode_ == RAW) 
			delete[] raw_buf_;
	}
	
	void CreateConn(int argc, char *argv[], const char *optstring);
	void ParseArg(int argc, char *argv[], const char *optstring);
	void InitSock();
	void HandShake();
	void ConfigFD();
	uint16 Read(IOType type, char *buf, uint16 len);
	uint16 Write(IOType type, char *buf, uint16 len, int next_radio);
	int AllocTun(char *dev, int flags);
	int CreateSock(int sock_type);
	void CreateAddr(const char *ip, int port, struct sockaddr_in *addr);
	void Connect(int sock_fd, int sock_type, const struct sockaddr_in *server_addr);
	int Accept(int listen_fd, int sock_type, struct sockaddr_in *client_addr);

	int GetNextRadio();

// Data members:
  	int tun_fd_;
  	int tun_type_;    		// TUN or TAP
  	int comm_mode_;   		// TCP or UDP
  	int server_client_mod_;   // server or client
  	char if_name_[IFNAMSIZ];
  	char server_ip_eth_[16];

  	struct sockaddr_in server_addr_eth_, client_addr_eth_; 
  	uint16 port_eth_;
  	int sock_fd_eth_;		// Sockets to handle request at the server side
  	int net_fd_eth_;		// Sockets to handle data
	int net_rd_;
	int maxfd_;
	// For raw socket only
	char *raw_buf_;
	char dev_[IFNAMSIZ];


	int radio_mode_;
	int radio_count_;

	RadioContext ath_[MAX_RADIO];

	bool flow_control_;

#ifdef LOG_PKT
	//FILE *fp_log_seq_;
	FILE *fp_log_payload_;
#endif
};

inline void Tun::CreateConn(int argc, char *argv[], const char *optstring)
{
	ParseArg(argc, argv, optstring); 
	InitSock();
	HandShake();
	ConfigFD();
}

int cread(int fd, char *buf, int n);
int cwrite(int fd, char *buf, int n);
int read_n(int fd, char *buf, int n);

#endif
