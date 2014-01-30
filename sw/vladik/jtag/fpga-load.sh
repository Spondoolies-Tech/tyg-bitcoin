#!/bin/sh
#
# Written by Vladik Goytin

#	Signal		I/O line	GPIO		Direction
#	=========================================================
#	TCK		GPIO2_25	89		out
#	TMS		GPIO2_24	88		out
#	TDI		GPIO2_22	86		out
#	TDO		GPIO3_21	117		in
#
#	FPGA_PROGRAM_N	GPIO2_23	87		out, low
#	FPGA_RESET	GPIO1_15	47		out, 0 then 1
#


GPIO_CLASS=/sys/class/gpio

TCK_GPIO=89
TMS_GPIO=88
TDI_GPIO=86
TDO_GPIO=117
FPGA_PROGRAM_N=87
FPGA_RESET=47


# $1 -- GPIO
# $2 -- value
gpio_out()
{
	echo $1 > ${GPIO_CLASS}/export
	echo out > ${GPIO_CLASS}/gpio$1/direction
	echo $2 > ${GPIO_CLASS}/gpio$1/value
}

# $1 -- GPIO
gpio_in()
{
	echo $1 > ${GPIO_CLASS}/export
	echo in > ${GPIO_CLASS}/gpio$1/direction
}

configure_pins()
{
	gpio_out ${FPGA_PROGRAM_N} 0

	gpio_out ${TCK_GPIO} 0
	gpio_out ${TMS_GPIO} 1
	gpio_out ${TDI_GPIO} 1
	gpio_in  ${TDO_GPIO}
}

release_pins()
{
	echo ${TCK_GPIO} > ${GPIO_CLASS}/unexport
	echo ${TMS_GPIO} > ${GPIO_CLASS}/unexport
	echo ${TDI_GPIO} > ${GPIO_CLASS}/unexport
	echo ${TDO_GPIO} > ${GPIO_CLASS}/unexport
	echo ${FPGA_PROGRAM_N} > ${GPIO_CLASS}/unexport

	# FPGA reset pin must remain active otherwise FPGA is not working.
	# Vladik, 16.01.2014
}

reset_fpga()
{
	echo ${FPGA_RESET} > ${GPIO_CLASS}/unexport
	gpio_out ${FPGA_RESET} 0
	echo 1 > ${GPIO_CLASS}/gpio${FPGA_RESET}/value
}

main()
{
	configure_pins
	jam -aconfigure -z1 "$@"
	reset_fpga
	release_pins
}

main $@
