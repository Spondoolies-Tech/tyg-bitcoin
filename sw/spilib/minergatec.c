#include "minergate.h"
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>





void send_minergate_pkt(const minergate_packet* mp_req, 
						minergate_packet* mp_rsp,
						int  socket_fd) {
	int 	nbytes;		
	write(socket_fd, (const void*)mp_req, mp_req->data_length + MINERGATE_PACKET_HEADER_SIZE);
	nbytes = read(socket_fd, (void*)mp_rsp, 10000);					
	assert(mp_rsp->magic == 0xcafe);
	assert(mp_rsp->data_length + MINERGATE_PACKET_HEADER_SIZE == nbytes);
}


int init_socket() {
	struct sockaddr_un address;
	int  socket_fd ;
	socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	 if(socket_fd < 0)
	 {
	  printf("socket() failed\n");
	  perror("Err:");
	  return 0;
	 }
	 
	 /* start with a clean address structure */
	 memset(&address, 0, sizeof(struct sockaddr_un));
	 
	 address.sun_family = AF_UNIX;
	 sprintf(address.sun_path, MINERGATE_SOCKET_FILE);
	 
	 if(connect(socket_fd, 
				(struct sockaddr *) &address, 
				sizeof(struct sockaddr_un)) != 0)
	 {
	  printf("connect() failed\n");
	  perror("Err:");
	  return 0;
	 }

	return socket_fd;
}


int mdh(minergate_data* md, void* c, void* c2) {
	printf("Got %d %d %d\n",md->data_id,md->data_length,md->magic);
}



int main(void)
{
 
 int  socket_fd = init_socket();
 assert(socket_fd != 0);
 
 minergate_packet* mp_req = allocate_minergate_packet(10000, 0xca, 0xfe);
 //char* buffer = mp_req;
 minergate_data* md1 =	get_minergate_data(mp_req,	100, 1);
 minergate_data* md2 =	get_minergate_data(mp_req,	200, 2);

 

 //nbytes = snprintf(buffer, 256, "hello from a client");
 char buffer[10000];
 minergate_packet* mp_rsp = (void*)buffer;
 send_minergate_pkt(mp_req,  mp_rsp, socket_fd);
 printf("MESSAGE FROM SERVER: %x\n", mp_rsp->request_id);
 parse_minergate_packet(mp_rsp, mdh, NULL, NULL);
 close(socket_fd);
 free(mp_req);
 return 0;
}

/*

int main(int argc, char *argv[]) {
	minergate_packet *mp = allocate_minergate_packet(1000, 1, 2);
	minergate_data* md1 =  get_minergate_data(mp,  100, 1);
	minergate_data* md2 =  get_minergate_data(mp,  200, 2);
	minergate_data* md3 =  get_minergate_data(mp,  300, 3);
	printf("Got:\n");

	parse_minergate_packet(mp, mdh);

	return 0;
}
*/


