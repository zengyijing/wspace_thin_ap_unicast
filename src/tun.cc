#include "tun.h"

static int radio_index = 0;
static float send_time = 0;
static int send_count = 0;
int main(int argc, char **argv)
{
	setvbuf(stdout, (char *) NULL, _IONBF, 0);
	Tun tun;
	tun.ParseArg(argc, argv, "i:d:S:s:p:r:m:f");
	tun.InitSock();
	tun.HandShake();
	tun.ConfigFD();
	char buffer[PKT_SIZE], buffer1[PKT_SIZE+ATH_DATA_HEADER_SIZE];
	uint16 nread, nwrite;
	unsigned long int tap2net = 0, net2tap = 0;
	AthDataHeader hdr[MAX_RADIO];
	printf("sizeof(AthDataHeader): %u\n", sizeof(AthDataHeader));
	/*
	for(int i = 0; i< tun.radio_count_; i++)
	{
		hdr[i].SetRate((unsigned short)tun.ath_[i].rate_);
		printf("GetRate: %gMbps\n", hdr[i].GetRate()*1.0/10);
	}
	*/
	while(1)
	{
		int ret;
		fd_set rd_set;

		FD_ZERO(&rd_set);
		/*
		FD_SET(tun.tun_fd_, &rd_set);
		*/
		FD_SET(tun.net_rd_, &rd_set);

		ret = select(tun.maxfd_ + 1, &rd_set, NULL, NULL, NULL);
		if (ret < 0 && errno == EINTR)
		{
			continue;
		}

		if (ret < 0)
		{
			perror("select()");
			exit(-1);
		}
		/*
		if (FD_ISSET(tun.tun_fd_, &rd_set))
		{
			nread = tun.Read(Tun::kTun, buffer, PKT_SIZE);
			tap2net++;
			//printf("TAP2NET %lu: Read %d bytes from the tun interface\n", tap2net, nread);

			int index = tun.GetNextRadio();
			hdr[index].seq_num_++;
			//TIME::GetCurrTime(&(hdr.time_));
			memcpy(buffer1, (void*)&hdr[index], sizeof(AthDataHeader));
			memcpy((void*)(buffer1+sizeof(AthDataHeader)), buffer, nread);
			nwrite = tun.Write(Tun::kNet, buffer1, nread+sizeof(AthDataHeader), index);
			if (nwrite < 0)
			{
				for(int i = 0; i < tun.radio_count_; i++)
				{
					close(tun.ath_[i].net_wr_);
				}
				Perror("tun.Write net Error!\n");
				exit(-1);
			}
			//printf("TAP2NET %lu: %d bytes\n", tap2net, nwrite);

			//
			if (tun.flow_control_ == true)
			{
				uint32 kExtraWaitTime = 140;//DIFS_80211ag + SLOT_TIME * 3;
				uint32 pkt_duration = ((nwrite+36)*8.0)/(tun.ath_[index].rate_/10.0*(tun.ath_[index].chanbw_/20.0))-kExtraWaitTime;
				usleep(pkt_duration);
			}
			//
		}
		*/
		if(FD_ISSET(tun.net_rd_, &rd_set))  /** From Ethernet. */
		{
			nread = tun.Read(Tun::kNet, buffer, PKT_SIZE);
			if (nread == 0)
			{
				break;
			}
			net2tap++;
			/*
			//printf("NET2TAP %lu: AP reads %d bytes from net\n", net2tap, nread);  // To do: change the interface later
			nwrite = tun.Write(Tun::kTun, &(buffer[sizeof(AthDataHeader)]), nread-sizeof(AthDataHeader), 0);
			//printf("NET2TAP %lu: Written %d bytes to the tun interface\n", net2tap, nwrite);
			*/
			int index = tun.GetNextRadio();
			nwrite = tun.Write(Tun::kNet, buffer, nread, index);
			if (nwrite < 0)
			{
				for(int i = 0; i < tun.radio_count_; i++)
				{
					close(tun.ath_[i].net_wr_);
				}
				Perror("tun.Write net Error!\n");
				exit(-1);
			}
		}
	}
	return 0;
}

int Tun::GetNextRadio()
{
	if(radio_mode_ > 0)
		return radio_mode_ - 1;
	else
	{
		int next = radio_index++;
		if(radio_index == radio_count_)
			radio_index = 0;
		return next;
	}
}

void Tun::ParseArg(int argc, char *argv[], const char *optstring)
{
	/* Check command line options */
	int option;
	while ((option = getopt(argc, argv, optstring)) > 0)
	{
		switch(option)
		{
			/*
			case 'i':
			{
				strncpy(if_name_, optarg, IFNAMSIZ-1);
				if (!strncmp(if_name_, "tun0", 3))
				{
					// printf("Tun interface!\n");
					tun_type_ = IFF_TUN;
				}
				else
				{
					Perror("Invalid tun iface: %s\n", optarg);
				}
				break;
			}
			case 'd':
			{
				strncpy(dev_, optarg, IFNAMSIZ-1);
				break;
			}
			*/
			case 'S':
			{
				server_client_mod_ = SERVER;
				strncpy(server_ip_eth_,optarg,16);
				printf("server_ip_eth: %s\n", server_ip_eth_);
				break;
			}
			case 's':
			{
				server_client_mod_= SERVER;
				char buff[PKT_SIZE] = {0};
				strcpy(buff,optarg);
				char* p = strtok(buff,",");
				strncpy(ath_[0].server_ip_ath_, p, 16);
				printf("ath_[0].server_ip_ath_: %s\n", ath_[0].server_ip_ath_);
				int count = 1;
				while(p = strtok(NULL,","))
				{
					strncpy(ath_[count].server_ip_ath_, p, 16);
					printf("ath_[%d].server_ip_ath_: %s\n", count, ath_[count].server_ip_ath_);
					count++;
				}
				if (radio_count_ == 0)
					radio_count_ = count;
				else if (radio_count_ != count)
					Perror("number of radio do not match\n");
				break;
			}
			/*
			case 'C':
			{
				server_client_mod_ = CLIENT;
				strncpy(server_ip_eth_,optarg,16);
				printf("server_ip_eth: %s\n", server_ip_eth_);
				break;
			}
			case 'c':
			{
				server_client_mod_ = CLIENT;
				strncpy(server_ip_ath_,optarg,16);
				printf("server_ip_ath: %s\n", server_ip_ath_);
				break;
			}
			*/
			/*
			case 'r':  // For data rate
			{
				char buff[PKT_SIZE] = {0};
				strcpy(buff,optarg);
				char* p = strtok(buff,",");
				ath_[0].rate_ = atoi(p);
				int count = 1;
				while(p = strtok(NULL, ","))
				{
					ath_[count++].rate_ = atoi(p);
				}
				if (radio_count_ == 0)
					radio_count_ = count;
				else if (radio_count_ != count)
					Perror("number of radio do not match\n");

				break;
			}
			*/
			case 'p':
			{
				if (!strcmp(optarg, "tcp"))
				{
					comm_mode_ = TCP;
				}
				else if (!strcmp(optarg, "udp"))
				{
					comm_mode_ = UDP;
				}
				else
				{
					comm_mode_ = RAW;
				}
				break;
			}
			case 'm':
			{
				radio_mode_ = atoi(optarg);
				break;
			}
			case 'f':
			{
				flow_control_ = true;
				break;
			}
			default:
			{
				Perror("Usage: %s -S server_eth_ip -s server_ath_ips -p tcp/udp\n", argv[0]);
			}
		}
	}

	// Broadcast ip
	for(int i =0; i< radio_count_; i++)
	{
		strcpy(ath_[i].broadcast_ip_ath_, ath_[i].server_ip_ath_);
		char* p = strrchr(ath_[i].broadcast_ip_ath_, '.');
		strcpy(p + 1, "255");
		printf("ath_[%d].broadcast_ip_ath_: %s\n", i, ath_[i].broadcast_ip_ath_);
	}
	/*
	if (strlen(if_name_) == 0)
	{
		Perror("Must specify interface name!\n");
  	}
	*/
	if (server_client_mod_ == INVALID_MODE)
	{
		Perror("Must specify client or server mode!\n");
  	}
	if (strlen(server_ip_eth_) == 0)
	{
		Perror("Must specify eth address!\n");
  	}
	for(int i = 0; i < radio_count_; i++)
	{
		if (strlen(ath_[i].server_ip_ath_) == 0)
			Perror("Must specify ath address!\n");
	}

	if (radio_count_ > MAX_RADIO)
	{
		Perror("Cannot support that many links!\n");
	}

	if (radio_mode_ > radio_count_)
	{
		Perror("Invalid radio mode!\n");
	}

  	if (comm_mode_ == TCP)
	{
		printf("Protocol: TCP\n");
  	}
	else if (comm_mode_ == UDP)
	{
		printf("Protocol: UDP\n");
	}
	else
	{
		printf("Protocl: Raw socket\n");
		if (strlen(dev_) == 0)
		{
			Perror("Raw sock has to specify wlan iface name!\n");
		}
		else
		{
			printf("dev: %s\n", dev_);
		}
	}
	/*
	// For rate
	for(int i = 0; i < radio_count_; i++)
	{
		assert(ath_[i].rate_ != 0);
	}
	*/
	/*
	//get mac address using ifconfig
	for(int i = 0; i < radio_count_; i++)
	{
		char cmd[16] = {0};
		sprintf(cmd, "ifconfig wlan%d", i );
		FILE * pipe = popen(cmd, "r");
		if(pipe == NULL)
		{
			perror("popen");
			exit(-1);
		}
		char buffer[128] = {0};
		char mac_addr[18] = {0};
		char ip_addr[16] = {0};
		fgets (buffer, 128, pipe);
		char * p = strstr(buffer, "HWaddr ");
		if (p != NULL)
		{
			strncpy(mac_addr, p + strlen("HWaddr "), 17);
			fgets(buffer, 128, pipe);
			strncpy(ip_addr, strstr(buffer, "inet addr:") + strlen("inet addr:"), 15);
			char * q = strstr(ip_addr, " ");
			if(q != NULL)
				*q = '\0';
			for (int j = 0; j < radio_count_; j++)
			{
				//printf("ath_[%d].server_ip_ath_:%s\n", j, ath_[j].server_ip_ath_);
				if (strcmp(ath_[j].server_ip_ath_, ip_addr) == 0)
				{
					strcpy(ath_[j].mac_addr_ath_, mac_addr);
					break;
				}
			}
			
		}
		pclose(pipe);
	}

	//get channel, chanbw in /etc/config/wireless using mac address
	for(int i = 0; i < radio_count_; i++ )
	{
		FILE * stream;    // Input and output file descriptors
		char buffer[128]={0};      // Character buffer 
		bzero(buffer, sizeof(buffer));
		// Create input file descriptor
		stream = fopen ("/etc/config/wireless", "r");
		if (stream == NULL)
		{
			perror ("fopen");
			exit(-1);
		}

		// find mac_address and get channel, chanbw
		while(fgets (buffer, 128, stream) != NULL)
		{
			bool escape = false;

			if(strcasestr(buffer, ath_[i].mac_addr_ath_) != NULL)
			{
				bool channel = false;
				bool chanbw = false;
				while(fgets(buffer, 128, stream) != NULL)
				{
					char * p = strstr(buffer, "channel");
					if(p != NULL)
					{
						p += strlen("channel ");
						if(atoi(p) == 0)
							p++;
						ath_[i].channel_ = atoi(p);
						channel = true;
					}
					if((p = strstr(buffer, "chanbw")) != NULL)
					{
						p += strlen("chanbw ");
						if(atoi(p) == 0)
							p++;
						ath_[i].chanbw_ = atoi(p);
						chanbw = true;
					}
					if(channel && chanbw)
					{
						escape = true;
						break;
					}
				}
			}

			if(escape)
				break;
		}
		// Close file descriptors
		fclose(stream);
		printf("ath_[%d].channel_:%d, ath_[%d].chanbw_:%d\n", i, ath_[i].channel_, i, ath_[i].chanbw_);
	}
	*/
}

int Tun::AllocTun(char *dev, int flags)
{
	struct ifreq ifr;
	int fd, err;
	char *clonedev = "/dev/net/tun";

	if( (fd = open(clonedev , O_RDWR)) < 0 )
	{
		perror("Opening /dev/net/tun");
		return fd;
	}

	memset(&ifr, 0, sizeof(struct ifreq));

	ifr.ifr_flags = flags;

	if (*dev)
	{
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  	}

	if((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 )
	{
		perror("ioctl(TUNSETIFF)");
		close(fd);
		return err;
	}

	strcpy(dev, ifr.ifr_name);
	return fd;
}

void Tun::CreateAddr(const char *ip, int port, struct sockaddr_in *addr)
{
	memset(addr, 0, sizeof(sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = inet_addr(ip);
	addr->sin_port = htons(port);
}

void Tun::InitSock()
{
	/* initialize tun/tap interface */
	/*
	if ( (tun_fd_ = AllocTun(if_name_, tun_type_ | IFF_NO_PI)) < 0 )
	{
		Perror("Error connecting to tun/tap interface %s!\n", if_name_);
		exit(1);
	}
	*/

	// Create sockets
	sock_fd_eth_ = CreateSock(UDP);
	for(int i = 0; i < radio_count_; i++)
	{
		ath_[i].sock_fd_ath_ = CreateSock(comm_mode_);  // should be broadcast at the server side
	}
	// Create server side address
	CreateAddr(server_ip_eth_, port_eth_, &server_addr_eth_);
	for(int i = 0; i < radio_count_; i++)
	{
		CreateAddr(ath_[i].server_ip_ath_, ath_[i].port_ath_, &ath_[i].server_addr_ath_);
	}
	int optval = 1;
	if(setsockopt(sock_fd_eth_, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(int)) < 0)
	{
		perror("eth setsockopt()");
		exit(-1);
	}

	for(int i = 0; i < radio_count_; i++)
	{
		if(setsockopt(ath_[i].sock_fd_ath_, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(int)) < 0)
		{
			perror("ath setsockopt()");
			exit(-1);
		}
	}
	// Server binds to eth sock
	socklen_t addr_len = sizeof(struct sockaddr_in);
	if (bind(sock_fd_eth_, (struct sockaddr*)&server_addr_eth_, addr_len) < 0)
	{
		perror("eth bind()");
	}
	// Configure ath sock
	// Server binds to ath sock
	if (comm_mode_ == UDP)
	{
		for(int i = 0; i < radio_count_; i++)
		{
			if (bind(ath_[i].sock_fd_ath_, (struct sockaddr*)&ath_[i].server_addr_ath_, addr_len) < 0)
			{
				perror("ath bind()");
			}
		}
	}
	else
		assert(0);

	// Server sets the ath sock to be broadcast sock
	int is_broadcast=1;
	for(int i = 0; i < radio_count_; i++)
	{
		if (setsockopt(ath_[i].sock_fd_ath_, SOL_SOCKET, SO_BROADCAST, &is_broadcast, sizeof(int)) < 0)
		{
			close(ath_[i].sock_fd_ath_);
			perror("Error: InitSock set broadcast option fails!\n");
		}
	}
}

int Tun::CreateSock(int sock_type)
{
	int sock_fd;
	if (sock_type == TCP)
	{
		if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			perror("TCP socket()");
			exit(-1);
		}
	}
	else if (sock_type == UDP)
	{
		if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		{
			perror("UDP socket()");
			exit(-1);
		}
	}
	else
	{
		if ((sock_fd = socket(PF_PACKET, SOCK_RAW, ETH_P_ALL)) < 0)
		{
			perror("Raw socket()");
			exit(-1);
		}
	}
	return sock_fd;
}

void Tun::Connect(int sock_fd, int sock_type, const struct sockaddr_in *server_addr)
{
	if (sock_type == TCP)
	{
		if (connect(sock_fd, (struct sockaddr*)server_addr, sizeof(struct sockaddr_in)) < 0)
		{
			fprintf(stderr, "connect to %s fails\n", inet_ntoa(server_addr->sin_addr));
			exit(-1);
		}
	}
	else
	{
		char buffer[PKT_SIZE];
		snprintf(buffer, PKT_SIZE, "connect\0");
		if (sendto(sock_fd, buffer, strlen(buffer)+1, 0, (struct sockaddr*)server_addr, sizeof(struct sockaddr_in)) == -1)
		{
			Perror("UDP sendto fails! Exit!\n");
			exit(-1);
		}
		if (recvfrom(sock_fd, buffer, PKT_SIZE, 0, NULL, NULL) == -1)
		{
			Perror("Recvfrom is dead!");
			exit(-1);
		}
		// printf("buffer: %s\n", buffer);
		if (strcmp(buffer, "accept") != 0)
		{
			Perror("Invalid connection message from server: %s Exit...\n", buffer);
			exit(-1);
		}
	}
}

int Tun::Accept(int listen_fd, int sock_type, struct sockaddr_in *client_addr)
{
	int net_fd;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	bzero((char*)client_addr, (int)addr_len);
	if (sock_type == TCP)
	{
		if (listen(listen_fd, 5) < 0)
		{
			perror("listen()");
			exit(-1);
		}
			
		if ((net_fd = accept(listen_fd, (struct sockaddr*)client_addr, &addr_len)) < 0)
		{
			perror("accept()");
			exit(-1);
		} 

	}
	else // UDP mode
	{
		char buffer[PKT_SIZE];
		net_fd = listen_fd;
		if (recvfrom(listen_fd, buffer, PKT_SIZE, 0, (struct sockaddr*)client_addr, (socklen_t*)&addr_len) == -1)
		{
			Perror("Server recvfrom fail!\n");
			exit(-1);
		}
		// printf("buffer: %s\n", buffer);
		if (strcmp(buffer, "connect") != 0)
		{
			Perror("Invalid connection message from client: %s Exit...\n", buffer);
			exit(-1);
		}
		snprintf(buffer, PKT_SIZE, "accept\0");
		// Server reply
		if (sendto(listen_fd, buffer, strlen(buffer)+1, 0, (struct sockaddr*)client_addr, (socklen_t)addr_len) == -1)
		{
			Perror("Server recvfrom fail!\n");
			exit(-1);
		}
	}
	return net_fd;
}

void Tun::HandShake()
{
	/*	
	net_fd_eth_ = Accept(sock_fd_eth_, comm_mode_, &client_addr_eth_);
	printf("SERVER: Connected from client: %s\n", inet_ntoa(client_addr_eth_.sin_addr));
	*/
	for(int i = 0; i < radio_count_; i++)
	{
		ath_[i].net_fd_ath_ = ath_[i].sock_fd_ath_;
	}
	net_fd_eth_ = sock_fd_eth_;
	printf("Ready to broadcast the packets from server\n");
}

void Tun::ConfigFD()
{
	/*
	maxfd_ = (tun_fd_ > net_fd_eth_) ? tun_fd_ : net_fd_eth_;
	*/
	maxfd_ = net_fd_eth_;
	net_rd_ = net_fd_eth_;
	for(int i = 0; i < radio_count_; i++)
	{
		ath_[i].net_wr_ = ath_[i].net_fd_ath_;
		CreateAddr(ath_[i].broadcast_ip_ath_, ath_[i].broadcast_port_ath_, &ath_[i].remote_);
	}
}

uint16 Tun::Read(IOType type, char *buf, uint16 len)
{
	uint16 nread=-1;
	if (type == kTun)
	{
		nread = cread(tun_fd_, buf, len);
	}
	else  // pkt from network iface
	{
		if (comm_mode_ == TCP)
		{
			uint16 plength;
			nread = read_n(net_rd_, (char *)&plength, sizeof(uint16));
		  	if(nread > 0)
			{
				nread = read_n(net_rd_, buf, ntohs(plength));
			} 
		}
		else
		{
			nread = recvfrom(net_rd_, buf, len, 0, NULL, NULL);
		}	
	}
	return nread;
}

uint16 Tun::Write(IOType type, char *buf, uint16 len, int index)
{
	uint16 nwrite=-1;
	if (type == kTun)
	{
		nwrite = cwrite(tun_fd_, buf, len);
	}
	else
	{
		if (comm_mode_ == TCP)
		{
			uint16 plength = htons(len);
			nwrite = cwrite(ath_[index].net_wr_, (char*)&plength, sizeof(uint16));
			nwrite = cwrite(ath_[index].net_wr_, buf, len);
	  	}
		else if (comm_mode_ == UDP)
		{/*
			struct timeval te1, te2;
			gettimeofday(&te1, NULL); // get current time
		*/
			nwrite = sendto(ath_[index].net_wr_, buf, len, 0, (struct sockaddr*)&ath_[index].remote_, sizeof(struct sockaddr_in));
		/*	
			gettimeofday(&te2, NULL); // get current time
			send_time += (te2.tv_sec - te1.tv_sec) * 1000000 + te2.tv_usec - te1.tv_usec;
			send_count++;
			if(send_count%1000 == 0)
				printf("avg_send_time:%8f\n", send_time/send_count);
		*/
		
		}
		else
			assert(0);
	}
	return nwrite;
}

/**************************************************************************
 * cread: read routine that checks for errors and exits if an error is	*
 *		returned.													   *
 **************************************************************************/
inline int cread(int fd, char *buf, int n)
{
	int nread;
	if((nread=read(fd, buf, n)) < 0)
	{
		perror("Reading data");
	}
	return nread;
}

/**************************************************************************
 * cwrite: write routine that checks for errors and exits if an error is  *
 *		 returned.													  *
 **************************************************************************/
inline int cwrite(int fd, char *buf, int n)
{
	int nwrite;
	if((nwrite=write(fd, buf, n)) < 0)
	{
		perror("Writing data");
	}
	return nwrite;
}

/**************************************************************************
 * read_n: ensures we read exactly n bytes, and puts them into "buf".	 *
 *		 (unless EOF, of course)										*
 **************************************************************************/
inline int read_n(int fd, char *buf, int n)
{
	int nread, left = n;
	while(left > 0)
	{
		if ((nread = cread(fd, buf, left)) == 0)
		{
			return 0 ;
		}
		else
		{
			left -= nread;
			buf += nread;
		}
	}
	return n;  
}
