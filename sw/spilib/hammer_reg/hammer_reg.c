#include "squid.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <netinet/in.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>

void push_hammer_read(uint32_t addr, uint32_t offset, uint32_t* p_value);
void squid_wait_hammer_reads();
void push_hammer_write(uint32_t addr, uint32_t offset, uint32_t value);
void squid_wait_hammer_reads();



int main(int argc, char *argv[]) {

  init_spi();
  
  if (argc < 3 ) {
	printf("Usage for squid read/write  : %s squid <h`addr> [h`value]\n" , argv[0]);
	printf(" for hammer read/write : %s hammer <h`addr> [h`value]\n" , argv[0]);
	return 0;
  }

  printf("Enabling loop\n");
 // write_spi(0x10, 0xffffff);
 // write_spi(0x11, 0xfffffe);


  if (argv[1][0] == 's') {
	  if (argc < 4) {
		// READ
		unsigned int addr = strtoul(argv[2], NULL, 16);
		unsigned int value = read_spi(addr);
		printf("[%x] => %x\n",addr,value);
      } else {
		unsigned int addr  = strtoul(argv[2], NULL, 16);
		unsigned int value = strtoul(argv[3], NULL, 16);
		write_spi(addr, value);
		printf("[%x] <= %x\n",addr,value);
	  }
	  
  } else if (argv[1][0] == 'h') {
	  int d1;

      uint32_t q_status;
      q_status = read_spi(ADDR_SQUID_PONG);
      passert((q_status == 0xDEADBEEF), "ERROR: no 0xdeadbeef in squid pong register!\n");


      int i = 0;
	  while (read_spi(ADDR_SQUID_STATUS) & BIT_STATUS_SERIAL_Q_RX_NOT_EMPTY) {
			 printf("read garbage %x\n", read_spi(ADDR_SQUID_SERIAL_READ));
             i++;

             if (i == 1000) {
                printf("GARBAGE OVERFLOW!\n");
                return(0);
             }
	  }
		  

  
	  if (argc < 5) {
		// READ
		unsigned int addr = strtoul(argv[2], NULL, 16);
		unsigned int offset = strtoul(argv[3], NULL, 16);
		unsigned int value;
		push_hammer_read( addr,  offset, &value);
		squid_wait_hammer_reads();
		printf("[%x:%x] => %x\n",addr,offset,value);
      } else {
		unsigned int addr  = strtoul(argv[2], NULL, 16);
		unsigned int offset = strtoul(argv[3], NULL, 16);
		unsigned int value = strtoul(argv[4], NULL, 16);
		push_hammer_write(addr, offset, value);
		squid_wait_hammer_reads();
		printf("[%x:%x] <= %x\n",addr,offset,value);
	  }
	  
  } else  {
	  printf("Usage for squid read/write  : %s squid <h`addr> [h`value]\n" , argv[0]);
      printf(" for hammer read/write : %s hammer <h`addr> [h`value]\n" , argv[0]);
	  return 0;
  }  

}

