#include "mg_proto_parser.h"
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <time.h> 
#include "assert.h"
#include "squid.h"


int leading_zeroes  = 0x20;
int jobs_per_time   = 50;
int jobs_period   = 50;



void send_minergate_pkt(const minergate_packet* mp_req, 
						minergate_packet* mp_rsp,
						int  socket_fd) {
	int 	nbytes;		
	write(socket_fd, (const void*)mp_req, mp_req->data_length + MINERGATE_PACKET_HEADER_SIZE);
	nbytes = read(socket_fd, (void*)mp_rsp, 40000);	
	assert(nbytes > 0);
	//printf("got %d(%d) bytes\n",mp_rsp->data_length, nbytes);
	assert(mp_rsp->magic == 0xcafe);
	assert((mp_rsp->data_length + MINERGATE_PACKET_HEADER_SIZE) == nbytes);
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


int minergate_data_processor(minergate_data* md, void* c, void* c2) {
	//printf("Got %d %d %d\n",md->data_id,md->data_length,md->magic);
    
    
    if (md->data_id == MINERGATE_DATA_ID_DO_JOB_RSP) {        
           //DBG(DBG_NET, "GOT minergate_do_job_req: %x/%x\n", sizeof(minergate_do_job_req), md->data_length);
           int array_size = md->data_length / sizeof(minergate_do_job_rsp);
           assert(md->data_length % sizeof(minergate_do_job_rsp) == 0);
           int i;
           for (i = 0; i < array_size; i++) { // walk the jobs
                //printf("j");
                minergate_do_job_rsp* work = ((minergate_do_job_rsp*)md->data) + i;
                if (work->winner_nonce) {
                    printf("!!!GOT minergate job rsp %08x %08x\n",work->work_id_in_sw,work->winner_nonce);
                }
           }
       }
}




void fill_random_work2(minergate_do_job_req* work) {
	static int id = 1;
	id++;
	memset(work, 0, sizeof(minergate_do_job_req));
	//work->
	work->midstate[0] = (rand()<<16) + rand()+rand();
	work->midstate[1] = (rand()<<16) + rand();
	work->midstate[2] = (rand()<<16) + rand();
	work->midstate[3] = (rand()<<16) + rand();
	work->midstate[4] = (rand()<<16) + rand();
	work->midstate[5] = (rand()<<16) + rand();
	work->midstate[6] = (rand()<<16) + rand();
	work->midstate[7] = (rand()<<16) + rand();
	work->work_id_in_sw = id%0xFFFF;
	work->mrkle_root = (rand()<<16) + rand();
    work->leading_zeroes = leading_zeroes;
	work->timestamp  = (rand()<<16) + rand();
	work->difficulty = (rand()<<16) + rand();
	//work_in_queue->winner_nonce
}


int main(int argc, char* argv[])
{
 if (argc == 4) {
    leading_zeroes = atoi(argv[1]);
    jobs_per_time= atoi(argv[2]);    
    jobs_period= atoi(argv[3]);        
 } else {
    printf("Usage: %s <leading zeroes> <jobs per send> <send period milli>\n", argv[0]);
    return 0;
 }
 

    
 int  socket_fd = init_socket();
 assert(socket_fd != 0);

 
 
 minergate_packet* mp_req = allocate_minergate_packet(10000, 0xca, 0xfe);
 //char* buffer = mp_req;
 minergate_data* md1 =	get_minergate_data(mp_req,	(jobs_per_time)*sizeof(minergate_do_job_req), MINERGATE_DATA_ID_DO_JOB_REQ);
 //minergate_data* md2 =	get_minergate_data(mp_req,	200, 2);
 int i;
 
 //nbytes = snprintf(buffer, 256, "hello from a client");
 char buffer[40000];
 srand (time(NULL));

 while (1) {
     minergate_packet* mp_rsp = (minergate_packet*)buffer;

     for (i = 0; i < (jobs_per_time); i++) {
        minergate_do_job_req* p = (minergate_do_job_req*)(md1->data);
        fill_random_work2(p+i);
     }
     
     //printf("MESSAGE TO SERVER: %x\n", mp_req->request_id);
     send_minergate_pkt(mp_req,  mp_rsp, socket_fd);
     //printf("MESSAGE FROM SERVER: %x\n", mp_rsp->request_id);
     parse_minergate_packet(mp_rsp, minergate_data_processor, NULL, NULL);
     usleep(jobs_period*1000);
 }
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

	parse_minergate_packet(mp, minergate_data_processor);

	return 0;
}
*/


