

#include "spond_lib.h"
#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <utility>
#include <iomanip>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>




using namespace std;

FINISHED_CB finished_cb;
FINISHED_CB won_cb;

static ifstream tty_file;
static ofstream tty_file_o;
static ofstream log_o;

pthread_t poll_thread;

pthread_cond_t     cmd_cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t    cmd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t    pm = PTHREAD_MUTEX_INITIALIZER;


int last_req_id = 0;
char last_resp[200];
int MAX_JOBS_IN_HW;


map<int, SPOND_WORK*> works_map;
pthread_mutex_t    works_map_mutex = PTHREAD_MUTEX_INITIALIZER;


void spond_get_board_state(int *free_slots, int *engine_running);


/* Does the reverse of bin2hex but does not allocate any ram */
bool spond_hex2bin(unsigned char *p, const char *hexstr, size_t len)
{
	bool ret = false;

	while (*hexstr && len) {
		char hex_byte[4];
		unsigned int v;

		if ((!hexstr[1])) {
			assert(0);
			return ret;
		}

		memset(hex_byte, 0, 4);
		hex_byte[0] = hexstr[0];
		hex_byte[1] = hexstr[1];

		if ((sscanf(hex_byte, "%x", &v) != 1)) {
			assert(0);
			return ret;
		}

		*p = (unsigned char) v;

		p++;
		hexstr += 2;
		len--;
	}

	if ((len == 0 && *hexstr == 0))
		ret = true;
	return ret;
}


void hw_won_work(int work_id, int nonce) {
	map<int, SPOND_WORK*>::iterator it;
	//printf("hw_finished_work %x!! \n", work_id);
	pthread_mutex_lock(&works_map_mutex);
	it = works_map.find(work_id);
	if (it == works_map.end()) {
		printf("WARNING: Unrequested job [%x] RESOLVED by HW\n", work_id);
		pthread_mutex_unlock(&works_map_mutex);
	} else {
	    //works_map.erase(it);
		SPOND_WORK* w = it->second;
    	pthread_mutex_unlock(&works_map_mutex);
		w->success++;
		w->winner_nonce = nonce;
		w->work_state = SPOND_WORK_STATE_FOUND_NONCE;

		if (won_cb) {
			won_cb(w);
		}
	}

}



void hw_finished_work(int work_id) {
	map<int, SPOND_WORK*>::iterator it;
	//printf("hw_finished_work %x!! \n", work_id);
	pthread_mutex_lock(&works_map_mutex);
	it = works_map.find(work_id);
	if (it == works_map.end()) {
		printf("WARNING: Unrequested job [%x] DONE by HW\n", work_id);
	} else {
		SPOND_WORK* w = it->second;
		w->work_state = SPOND_WORK_STATE_FINISHED;
		if (finished_cb) {
			finished_cb(w);
		}
		works_map.erase(it);
	}
	pthread_mutex_unlock(&works_map_mutex);
}


void proccess_response(string* srsp) {
	const char *rsp = (char*)srsp->c_str();
	int match = 0;
	int cmd_id;


	{
		int nonce;
		//printf("!!proccess_response %d \"%s\"!! \n", __LINE__, rsp);
		char hash[100];
		if (sscanf(rsp, "RSP[%x] WIN %x",&cmd_id,&nonce/*, hash*/) == 2) {
			printf("!!!!! WIN %x !!!!! %x\n",cmd_id, nonce);
			hw_won_work(cmd_id, nonce);
		}

		//printf("!!proccess_response %d!! \n", __LINE__);

		static char buff[200];
		if (sscanf(rsp, "RSP[%x] %[^\t\n]",&cmd_id, buff) == 2) {
			if (!memcmp(buff, "DONE",5)) {
				//printf("LOST!!!!! %x\n",cmd_id);
				hw_finished_work(cmd_id);
			}

			if (!memcmp(buff, "NO_SPACE",8)) {
				printf("NO_SPACE??? Application gave too much !! %x\n",cmd_id);
				assert(0);
			}

			if (!memcmp(buff, "OK",2)) {
				 pthread_mutex_lock(&cmd_mutex);
				 last_req_id = cmd_id;
				 strcpy(last_resp,  buff+3);
				 pthread_cond_broadcast(&cmd_cond);
				 pthread_mutex_unlock(&cmd_mutex);
			}

			//assert(memcmp(buff, "GARBAGE",6));
		}
		//printf("!!proccess_response %d!! \n", __LINE__);
	}

}


void spond_log(const char* p) {
	pthread_mutex_lock(&pm);
	log_o << p;
	pthread_mutex_unlock(&pm);
}



void* poll_board_thread(void* p) {
	string str;
	string prefix = "RSP";
	while(tty_file.good()) // To get you all the lines.
	{
	   getline(tty_file,str); // Saves the line in STRING.
	   if (str.length()) {
		if (str.substr(0, prefix.size()) == prefix) {
			log_o << "    BLAZE: " << str << endl; // Prints our STRING.
			proccess_response(&str);
		} else {
			log_o << "	  BLAZE: " << str << endl; // Prints our STRING.
		}

	   }
	}

 	cout<<"CONNECTION LOST" << endl;
	assert(0);
	return 0;
}


int send_cmd_to_board(unsigned int index, const char* command);



void spond_dropwork() {
	send_cmd_to_board(0x10, "STOP");
//	send_cmd_to_board(0x11, "STOP");
}



int spond_connect(const char* tty_dev, FINISHED_CB done_cb, WON_CB _won_cb) {
    finished_cb = done_cb;
    won_cb = _won_cb;
    tty_file.open (tty_dev);
    tty_file_o.open (tty_dev);

	pthread_mutex_init(&cmd_mutex, 0);
	pthread_mutex_init(&pm, 0);


	struct termios term;
	int fd = open(tty_dev, O_RDWR);
	if (fd <0) {
		perror("open TTY 1");
		exit(-1);
	}
	if (tcgetattr(fd, &term) != 0)  {
		perror("open TTY 2"); exit(-1);
	}


	printf("I-SPEED:%x\n", cfgetispeed(&term));
	printf("O-SPEED:%x\n", cfgetospeed(&term));
	printf("Iflags:%x\n", term.c_iflag);	// 1
	printf("Cflags:%x\n", term.c_cflag);	// 1cb7
	printf("Lflags:%x\n", term.c_lflag);	// 0

			/*
			I-SPEED:1007
			O-SPEED:1007
			Iflags:1
			Cflags:1cb7
			Lflags:0
			*/


    if (term.c_iflag & BRKINT)
      puts("BRKINT is set");
    else
      puts("BRKINT is not set");

    if (term.c_cflag & PARENB)
      puts("parity is used");
	else
      puts("parity is NOT used");

    if (term.c_lflag & ECHO)
      puts("ECHO is set");
    else
      puts("ECHO is not set");
    printf("The end-of-file character is x'%02x'\n", term.c_cc[VEOF]);
	/*
	term.c_cflag |= OPOST;
	term.c_cflag |= ONLCR;
	*/

        // UGLY - todo fix!

	term.c_iflag = 1;
	term.c_cflag = 0x1cb7;
	term.c_lflag = 0;



	cfsetspeed(&term, /*B921600*/ B115200);
	tcsetattr(fd, 0, &term);

	printf("I-SPEED:%x\n", cfgetispeed(&term));
	printf("O-SPEED:%x\n", cfgetospeed(&term));

    log_o.open ("run_log.txt");
    assert(log_o);
    assert(tty_file);
    assert(tty_file_o);
    pthread_create(&poll_thread, NULL, poll_board_thread, NULL);
    int engine_running;
    printf("READING HW QUEUE LEN...\n");
	send_cmd_to_board(0x12, "STOP");
//	send_cmd_to_board(0x11, "STOP");


    spond_get_board_state(&MAX_JOBS_IN_HW, &engine_running);
    if (engine_running) {
		printf("JOB ALREADY RUNNING ON HW, WAITING...\n");
		while (engine_running) {
			spond_get_board_state(&MAX_JOBS_IN_HW, &engine_running);
			usleep(100000);
		}
    }
    printf("HW QUEUE LEN = %d, reseting NONCE range\n", MAX_JOBS_IN_HW);
    return 1;
}

//
// returns how much space for WORK availible, -1 for error.
//
ostream& hex2(ostream& out)
{
    return out << hex << setw(2) << setfill('0');
}




int send_cmd_to_board(unsigned int index, const char* command) {
	pthread_mutex_lock(&pm);
	log_o << string("CMD[") << hex2 << index << "] " << command << endl;
	log_o.flush();
	tty_file_o<< string("CMD[") << hex2 << index << "] " << command << endl;
	tty_file_o.flush();
	pthread_mutex_lock(&cmd_mutex);
    last_req_id = 0;
 	struct timespec   ts;
  	struct timeval    tp;
	int rc;
	rc =  gettimeofday(&tp, NULL);
	ts.tv_sec  = tp.tv_sec;
    ts.tv_nsec = tp.tv_usec * 1000;
    ts.tv_sec += 2;
	rc = pthread_cond_timedwait(&cmd_cond, &cmd_mutex, &ts);

	if(rc == ETIMEDOUT) {
		printf("ERROR: BOARD Timeout on: %s\n", command);
		assert(0);
	}

	if (last_req_id != index) {
		printf("%x != %x\n",last_req_id,index);
		assert(last_req_id == index);
	}
	//printf("RESP PARAMS: %s\n",last_resp);
	pthread_mutex_unlock(&cmd_mutex);
	pthread_mutex_unlock(&pm);
}



int send_cmd_to_board_get_responce(unsigned int index, const char* command, char buffer[]) {

	pthread_mutex_lock(&pm);
	log_o << string("CMD[") << hex2 << index << "] " << command << endl;
	log_o.flush();
	tty_file_o<< string("CMD[") << hex2 << index << "] " << command << endl;
	tty_file_o.flush();
	pthread_mutex_lock(&cmd_mutex);
    last_req_id = 0;

	struct timespec   ts;
  	struct timeval    tp;
	int rc;
	rc =  gettimeofday(&tp, NULL);
	ts.tv_sec  = tp.tv_sec;
    ts.tv_nsec = tp.tv_usec * 1000;
    ts.tv_sec += 2;


	rc = pthread_cond_timedwait(&cmd_cond, &cmd_mutex, &ts);

	if(rc == ETIMEDOUT) {
		printf("ERROR: BOARD Timeout on: %s\n", command);
		assert(0);
	}


	assert(last_req_id == index);
	//printf("RESP PARAMS: %s\n",last_resp);
	strcpy(buffer, last_resp);
	pthread_mutex_unlock(&cmd_mutex);
	pthread_mutex_unlock(&pm);
}


static inline uint32_t default_swab32(uint32_t val)
{
	return (((val & 0xff000000) >> 24) |
		((val & 0x00ff0000) >>  8) |
		((val & 0x0000ff00) <<  8) |
		((val & 0x000000ff) << 24));
}



int spond_give_work(SPOND_WORK *work) {
	pthread_mutex_lock(&works_map_mutex);
	static int leading0 = 0;
	if (works_map.size() >= MAX_JOBS_IN_HW) {
		pthread_mutex_unlock(&works_map_mutex);
		return -1;
	}
	work->success = 0;
	works_map[work->work_id] = work;
	int space = MAX_JOBS_IN_HW - works_map.size();
	pthread_mutex_unlock(&works_map_mutex);
	char cmd[400];
	char middle[130];


	log_o << string("Leading:::") << (int)work->leading_zeroes << " - "<< leading0 << endl;
	log_o.flush();
	if (leading0 != work->leading_zeroes) {
		sprintf(cmd,"LEADING0 %02x",work->leading_zeroes) ;
		send_cmd_to_board(work->work_id, cmd) ;
		leading0 = work->leading_zeroes;
	}


	// TODO - HW bug fix
	if (work->leading_zeroes == 0) {
		work->leading_zeroes = 1;
	}
	bin2str(work->midstate, 32, middle);
	sprintf(cmd,"DO_WORK D64:%08x%08x%08x M:%s",
			 default_swab32(*(int*)(work->data_tail64)), default_swab32(*(int*)(work->data_tail64 + 4)), default_swab32(*(int*)(work->data_tail64 + 8)),
 			 middle) ;


	send_cmd_to_board(work->work_id, cmd) ;
	printf("-----------\n%s\n----------\n",cmd);
	return space;
}

static unsigned int current_cmd_index = 0x05;


// note str should have space for NULL.
void bin2str(const unsigned char *p, size_t p_len, char* str)
{
	int i;
	p_len*=2;

	for (i = 0; i < p_len; i++)
		sprintf(str + (i * 2), "%02x", (unsigned int) p[i]);
	str[p_len] = 0;
}




void spond_get_board_state(int *free_slots, int *engine_running) {
	int state = 0;
	char buffer[200];
	printf("Sending chipstate\n");
	send_cmd_to_board_get_responce(current_cmd_index++, "CHIPSTATE", buffer);
	printf("Got chipstate\n");
	sscanf(buffer, "%x", &state);
	*free_slots        =  state & 0xFFFF;
	*engine_running = state >> 16;
}


/*
void spond_set_target(const  unsigned char *target) {
	char c[160];
	char trg_s[32*2+2];
	bin2str(target, 32, trg_s);
	//sprintf(c, "TARGET_SET %s", trg_s);
	send_cmd_to_board(current_cmd_index++, c);
}
*/



