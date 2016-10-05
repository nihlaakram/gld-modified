/** $Id: controller.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
	@file auction.cpp
	@defgroup controller Transactive controller, OlyPen experiment style
	@ingroup market

 **/

#include "controller.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/errno.h>
#include <pthread.h>
//#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include <errno.h>
//#include <math.h>



#ifndef MYNODE_H_
#define MYNODE_H_
extern int averageClearingPrice;
#endif

#define MYNODE_H
int averageClearingPrice;
#define SOCKET int
#define INVALID_SOCKET (-1)
static pthread_t thread_id;
static int shutdown_server = 0; /**< flag to stop accepting incoming connections */
SOCKET sockfd = (SOCKET)0; /**< socket on which incomming connections are accepted */
static bool initStatus = false;

double enthousiasm;
double nashL;
double totalL;

double* indoorTemp;
double* outdoorTemp;
double* heatingSet;
double* coolingSet;
double* designHeatingCap;
double* designCoolingCap;
double* initHeatingCOP;
double* initCoolingCOP;
double* heatingD;
double* coolingD;
//void *serviceTheRequest(void *ptr);

CLASS* controller::oclass = NULL;

int i =0;
SOCKET newsockfd;
static pthread_t thread_id2;

controller::controller(MODULE *module){
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"controller",sizeof(controller),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class controller";
		else
			oclass->trl = TRL_QUALIFIED;

		if (gl_publish_variable(oclass,
			PT_enumeration, "simple_mode", PADDR(simplemode),
				PT_KEYWORD, "NONE", (enumeration)SM_NONE,
				PT_KEYWORD, "HOUSE_HEAT", (enumeration)SM_HOUSE_HEAT,
				PT_KEYWORD, "HOUSE_COOL", (enumeration)SM_HOUSE_COOL,
				PT_KEYWORD, "HOUSE_PREHEAT", (enumeration)SM_HOUSE_PREHEAT,
				PT_KEYWORD, "HOUSE_PRECOOL", (enumeration)SM_HOUSE_PRECOOL,
				PT_KEYWORD, "WATERHEATER", (enumeration)SM_WATERHEATER,
				PT_KEYWORD, "DOUBLE_RAMP", (enumeration)SM_DOUBLE_RAMP,
			PT_enumeration, "bid_mode", PADDR(bidmode),
				PT_KEYWORD, "ON", (enumeration)BM_ON,
				PT_KEYWORD, "OFF", (enumeration)BM_OFF,
				PT_KEYWORD, "PROXY", (enumeration)BM_PROXY,
			PT_enumeration, "use_override", PADDR(use_override),
				PT_KEYWORD, "OFF", (enumeration)OU_OFF,
				PT_KEYWORD, "ON", (enumeration)OU_ON,
			PT_double, "ramp_low[degF]", PADDR(ramp_low), PT_DESCRIPTION, "the comfort response below the setpoint",
			PT_double, "ramp_high[degF]", PADDR(ramp_high), PT_DESCRIPTION, "the comfort response above the setpoint",
			PT_double, "range_low", PADDR(range_low), PT_DESCRIPTION, "the setpoint limit on the low side",
			PT_double, "range_high", PADDR(range_high), PT_DESCRIPTION, "the setpoint limit on the high side",
			PT_char32, "target", PADDR(target), PT_DESCRIPTION, "the observed property (e.g., air temperature)",
			PT_char32, "setpoint", PADDR(setpoint), PT_DESCRIPTION, "the controlled property (e.g., heating setpoint)",
			PT_char32, "demand", PADDR(demand), PT_DESCRIPTION, "the controlled load when on",
			PT_char32, "load", PADDR(load), PT_DESCRIPTION, "the current controlled load",
			PT_char32, "total", PADDR(total), PT_DESCRIPTION, "the uncontrolled load (if any)",
			PT_char32, "market", PADDR(pMkt), PT_DESCRIPTION, "the market to bid into",
			PT_char32, "state", PADDR(state), PT_DESCRIPTION, "the state property of the controlled load",
			PT_char32, "avg_target", PADDR(avg_target),
			PT_char32, "std_target", PADDR(std_target),
			PT_double, "bid_price", PADDR(last_p), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the bid price",
			PT_double, "bid_quantity", PADDR(last_q), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the bid quantity",
			PT_double, "set_temp[degF]", PADDR(set_temp), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the reset value",
			PT_double, "base_setpoint[degF]", PADDR(setpoint0),
			// new stuff
			PT_double, "market_price", PADDR(clear_price), PT_DESCRIPTION, "the current market clearing price seen by the controller.",
			PT_double, "period[s]", PADDR(dPeriod), PT_DESCRIPTION, "interval of time between market clearings",
			PT_enumeration, "control_mode", PADDR(control_mode),
				PT_KEYWORD, "RAMP", (enumeration)CN_RAMP,
				PT_KEYWORD, "DOUBLE_RAMP", (enumeration)CN_DOUBLE_RAMP,
				PT_KEYWORD, "DEV_LEVEL", (enumeration)CN_DEV_LEVEL,
			PT_enumeration, "resolve_mode", PADDR(resolve_mode),
				PT_KEYWORD, "DEADBAND", (enumeration)RM_DEADBAND,
				PT_KEYWORD, "SLIDING", (enumeration)RM_SLIDING,
			PT_double, "slider_setting",PADDR(slider_setting),
			PT_double, "slider_setting_heat", PADDR(slider_setting_heat),
			PT_double, "slider_setting_cool", PADDR(slider_setting_cool),
			PT_char32, "override", PADDR(re_override),
			// double ramp
			PT_double, "heating_range_high[degF]", PADDR(heat_range_high),
			PT_double, "heating_range_low[degF]", PADDR(heat_range_low),
			PT_double, "heating_ramp_high", PADDR(heat_ramp_high),
			PT_double, "heating_ramp_low", PADDR(heat_ramp_low),
			PT_double, "cooling_range_high[degF]", PADDR(cool_range_high),
			PT_double, "cooling_range_low[degF]", PADDR(cool_range_low),
			PT_double, "cooling_ramp_high", PADDR(cool_ramp_high),
			PT_double, "cooling_ramp_low", PADDR(cool_ramp_low),
			PT_double, "heating_base_setpoint[degF]", PADDR(heating_setpoint0),
			PT_double, "cooling_base_setpoint[degF]", PADDR(cooling_setpoint0),
			PT_char32, "deadband", PADDR(deadband),
			PT_char32, "heating_setpoint", PADDR(heating_setpoint),
			PT_char32, "heating_demand", PADDR(heating_demand),
			PT_char32, "cooling_setpoint", PADDR(cooling_setpoint),
			PT_char32, "cooling_demand", PADDR(cooling_demand),
			PT_double, "sliding_time_delay[s]", PADDR(sliding_time_delay), PT_DESCRIPTION, "time interval desired for the sliding resolve mode to change from cooling or heating to off",
			PT_bool, "use_predictive_bidding", PADDR(use_predictive_bidding),
			
			// DEV_LEVEL
			PT_double, "device_actively_engaged", PADDR(is_engaged),
			PT_int32, "cleared_market", PADDR(last_market),  
			PT_int32, "locked", PADDR(locked),
			PT_double,"p_ON",PADDR(P_ON),
			PT_double,"p_OFF",PADDR(P_OFF),
			PT_double,"p_ONLOCK",PADDR(P_ONLOCK),
			PT_double,"p_OFFLOCK",PADDR(P_OFFLOCK),
			PT_double,"delta_u",PADDR(delta_u),
			PT_char32, "regulation_market_on", PADDR(pMkt2), PT_DESCRIPTION, "the willing to change state from ON->OFF market to bid into for regulation case",
			PT_char32, "regulation_market_off", PADDR(pMkt), PT_DESCRIPTION, "the willing to change state from OFF->ON market to bid into for regulation case",
			PT_double, "fast_regulation_signal[s]", PADDR(fast_reg_signal), PT_DESCRIPTION, "the regulation signal that the controller compares against (i.e., how often is there a control action",
			PT_double, "market_price_on", PADDR(clear_price2), PT_DESCRIPTION, "the current market clearing price seen by the controller in ON->OFF regulation market",
			PT_double, "market_price_off", PADDR(clear_price), PT_DESCRIPTION, "the current market clearing price seen by the controller  in OFF->ON regulation market",
			PT_double, "period_on[s]", PADDR(dPeriod2), PT_DESCRIPTION, "interval of time between market clearings in ON->OFF regulation market",
			PT_double, "period_off[s]", PADDR(dPeriod), PT_DESCRIPTION, "interval of time between market clearings in  OFF->ON regulation market",
			PT_int32, "regulation_period", PADDR(reg_period), PT_DESCRIPTION, "fast regulation signal period",
			PT_double, "r1",PADDR(r1), PT_DESCRIPTION, "temporary diagnostic variable",
			PT_double, "mu0",PADDR(mu0), PT_DESCRIPTION, "temporary diagnostic variable",
			PT_double, "mu1",PADDR(mu1), PT_DESCRIPTION, "temporary diagnostic variable",

			// redefinitions
			PT_char32, "average_target", PADDR(avg_target),
			PT_char32, "standard_deviation_target", PADDR(std_target),
			PT_int64, "bid_id", PADDR(bid_id),
#ifdef _DEBUG
			PT_enumeration, "current_mode", PADDR(thermostat_mode),
				PT_KEYWORD, "INVALID", (enumeration)TM_INVALID,
				PT_KEYWORD, "OFF", (enumeration)TM_OFF,
				PT_KEYWORD, "HEAT", (enumeration)TM_HEAT,
				PT_KEYWORD, "COOL", (enumeration)TM_COOL,
			PT_enumeration, "dominant_mode", PADDR(last_mode),
				PT_KEYWORD, "INVALID", (enumeration)TM_INVALID,
				PT_KEYWORD, "OFF", (enumeration)TM_OFF,
				PT_KEYWORD, "HEAT", (enumeration)TM_HEAT,
				PT_KEYWORD, "COOL", (enumeration)TM_COOL,
			PT_enumeration, "previous_mode", PADDR(previous_mode),
				PT_KEYWORD, "INVALID", TM_INVALID,
				PT_KEYWORD, "OFF", (enumeration)TM_OFF,
				PT_KEYWORD, "HEAT", (enumeration)TM_HEAT,
				PT_KEYWORD, "COOL", (enumeration)TM_COOL,
			PT_double, "heat_max", PADDR(heat_max),
			PT_double, "cool_min", PADDR(cool_min),
#endif
			PT_int32, "bid_delay", PADDR(bid_delay),
			PT_char32, "thermostat_state", PADDR(thermostat_state), PT_DESCRIPTION, "The name of the thermostat state property within the object being controlled",
			// PROXY PROPERTIES
			PT_double, "proxy_average", PADDR(proxy_avg),
			PT_double, "proxy_standard_deviation", PADDR(proxy_std),
			PT_int64, "proxy_market_id", PADDR(proxy_mkt_id),
			PT_int64, "proxy_market2_id", PADDR(proxy_mkt_id2),
			PT_double, "proxy_clear_price[$]", PADDR(proxy_clear_price),
			PT_double, "proxy_price_cap", PADDR(proxy_price_cap),
			PT_char32, "proxy_market_unit", PADDR(proxy_mkt_unit),
			PT_double, "proxy_initial_price", PADDR(proxy_init_price),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		memset(this,0,sizeof(controller));
	}
}

int controller::create(){
	memset(this, 0, sizeof(controller));
	sprintf((char *)(&avg_target), "avg24");
	sprintf((char *)(&std_target), "std24");
	slider_setting_heat = -0.001;
	slider_setting_cool = -0.001;
	slider_setting = -0.001;
	sliding_time_delay = -1;
	lastbid_id = -1;
	heat_range_low = -5;
	heat_range_high = 3;
	cool_range_low = -3;
	cool_range_high = 5;
	use_override = OU_OFF;
	period = 0;
	period2 = 0;
	use_predictive_bidding = FALSE;
	controller_bid.bid_id = -1;
	controller_bid.market_id = -1;
	controller_bid.price = 0;
	controller_bid.quantity = 0;
	controller_bid.state = BS_UNKNOWN;
	controller_bid.rebid = false;
	controller_bid.bid_accepted = true;
	controller_bid2.bid_id = -1;
	controller_bid2.market_id = -1;
	controller_bid2.price = 0;
	controller_bid2.quantity = 0;
	controller_bid2.state = BS_UNKNOWN;
	controller_bid2.rebid = false;
	controller_bid2.bid_accepted = true;
	bid_id = -1;
	return 1;
}

/** provides some easy default inputs for the transactive controller,
	 and some examples of what various configurations would look like.
 **/
void controller::cheat(){
	switch(simplemode){
		case SM_NONE:
			break;
		case SM_HOUSE_HEAT:
			sprintf(target, "air_temperature");
			sprintf(setpoint, "heating_setpoint");
			sprintf(demand, "heating_demand");
			sprintf(total, "total_load");
			sprintf(load, "hvac_load");
			sprintf(state, "power_state");
			ramp_low = -2;
			ramp_high = -2;
			range_low = -5;
			range_high = 0;
			dir = -1;
			break;
		case SM_HOUSE_COOL:
			sprintf(target, "air_temperature");
			sprintf(setpoint, "cooling_setpoint");
			sprintf(demand, "cooling_demand");
			sprintf(total, "total_load");
			sprintf(load, "hvac_load");
			sprintf(state, "power_state");
			ramp_low = 2;
			ramp_high = 2;
			range_low = 0;
			range_high = 5;
			dir = 1;
			break;
		case SM_HOUSE_PREHEAT:
			sprintf(target, "air_temperature");
			sprintf(setpoint, "heating_setpoint");
			sprintf(demand, "heating_demand");
			sprintf(total, "total_load");
			sprintf(load, "hvac_load");
			sprintf(state, "power_state");
			ramp_low = -2;
			ramp_high = -2;
			range_low = -5;
			range_high = 3;
			dir = -1;
			break;
		case SM_HOUSE_PRECOOL:
			sprintf(target, "air_temperature");
			sprintf(setpoint, "cooling_setpoint");
			sprintf(demand, "cooling_demand");
			sprintf(total, "total_load");
			sprintf(load, "hvac_load");
			sprintf(state, "power_state");
			ramp_low = 2;
			ramp_high = 2;
			range_low = -3;
			range_high = 5;
			dir = 1;
			break;
		case SM_WATERHEATER:
			sprintf(target, "temperature");
			sprintf(setpoint, "tank_setpoint");
			sprintf(demand, "heating_element_capacity");
			sprintf(total, "actual_load");
			sprintf(load, "actual_load");
			sprintf(state, "power_state");
			ramp_low = -2;
			ramp_high = -2;
			range_low = 0;
			range_high = 10;
			break;
		case SM_DOUBLE_RAMP:
			sprintf(target, "air_temperature");
			sprintf((char *)(&heating_setpoint), "heating_setpoint");
			sprintf((char *)(&heating_demand), "heating_demand");
			sprintf((char *)(&heating_total), "total_load");		// using total instead of heating_total
			sprintf((char *)(&heating_load), "hvac_load");			// using load instead of heating_load
			sprintf((char *)(&cooling_setpoint), "cooling_setpoint");
			sprintf((char *)(&cooling_demand), "cooling_demand");
			sprintf((char *)(&cooling_total), "total_load");		// using total instead of cooling_total
			sprintf((char *)(&cooling_load), "hvac_load");			// using load instead of cooling_load
			sprintf((char *)(&deadband), "thermostat_deadband");
			sprintf(load, "hvac_load");
			sprintf(total, "total_load");
			heat_ramp_low = -2;
			heat_ramp_high = -2;
			heat_range_low = -5;
			heat_range_high = 5;
			cool_ramp_low = 2;
			cool_ramp_high = 2;
			cool_range_low = 5;
			cool_range_high = 5;
			break;
		default:
			break;
	}
}


/** convenience shorthand
 **/
void controller::fetch_double(double **prop, char *name, OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	*prop = gl_get_double_by_name(parent, name);
	if(*prop == NULL){
		char tname[32];
		char *namestr = (hdr->name ? hdr->name : tname);
		char msg[256];
		sprintf(tname, "controller:%i", hdr->id);
		if(*name == NULL)
			sprintf(msg, "%s: controller unable to find property: name is NULL", namestr);
		else
			sprintf(msg, "%s: controller unable to find %s", namestr, name);
		throw(msg);
	}
}

void controller::fetch_int64(int64 **prop, char *name, OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	*prop = gl_get_int64_by_name(parent, name);
	if(*prop == NULL){
		char tname[32];
		char *namestr = (hdr->name ? hdr->name : tname);
		char msg[256];
		sprintf(tname, "controller:%i", hdr->id);
		if(*name == NULL)
			sprintf(msg, "%s: controller unable to find property: name is NULL", namestr);
		else
			sprintf(msg, "%s: controller unable to find %s", namestr, name);
		throw(msg);
	}
}

void controller::fetch_enum(enumeration **prop, char *name, OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	*prop = gl_get_enum_by_name(parent, name);
	if(*prop == NULL){
		char tname[32];
		char *namestr = (hdr->name ? hdr->name : tname);
		char msg[256];
		sprintf(tname, "controller:%i", hdr->id);
		if(*name == NULL){
			sprintf(msg, "%: controller unable to find property: name is NULL", namestr);
		}
		else
		{
			sprintf(msg, "%s: controller unable to find %s", namestr, name);
		}
		throw(msg);
	}
}



static pthread_t thread;

/** Receive data from the client (blocking)
	@returns the number of bytes received if successful, -1 if failed (errno is set).
 **/
static size_t recv_data(SOCKET s,char *buffer, size_t len)
{
#ifdef WIN32
	return (size_t)recv(s,buffer,(int)len,0);
#else
	return (size_t)read(s,(void *)buffer,len);
#endif
}

void *serviceTheRequest(void *dummyptr){
	//printf("--->1111\r\n");
	size_t len;
	//SOCKET fd = (SOCKET)*((int *)ptr);
	//int fd = (int)ptr;
	char* buffer = (char*)malloc(4096);
	char* buffer2 = (char*)malloc(4096);
	//printf("--->333\r\n");
	int counter =0;

	while ( (int)(len=recv_data(newsockfd,buffer,sizeof(buffer)))>0 )
	{
//		printf("==>|%s|<+++\r\n", buffer);

		if(strchr(buffer, '\r') != NULL){
			counter += len;
			strcat(buffer2, buffer);
			buffer2[counter] = '\0';
			if(strlen(buffer2) > 1){
				//printf(">>>>>>>>>>>>>>>>>: %s\r\n", buffer2);

				char* token = strtok(buffer2, " ");

				//nashL = atof(token);
				sscanf(token, "%lf", &nashL);
				printf("nash value : %.18f\r\n", nashL);
				token = strtok(NULL, " ");
				//totalL = atof(token);
				//double d;
				sscanf(token, "%lf", &totalL);
				printf("total Load : %.12f\r\n", totalL);
				//nashL = atof(buffer2);
				//printf("supp value : %f\r\n", nashL);

				printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");

			}
			memset(buffer2, 0, sizeof(buffer2));
			counter=0;
		}else{
			strncpy(buffer2+counter, buffer, len);
			counter += len;
		}
	}

	//free(buffer);
    //free(buffer2);
	//printf("--->2222\r\n");
}


/*
void *serviceTheRequest(void *dummyptr){
	//printf("--->1111\r\n");
	size_t len;
	//SOCKET fd = (SOCKET)*((int *)ptr);
	//int fd = (int)ptr;
	char* buffer = (char*)malloc(1024);
	char* buffer2 = (char*)malloc(1024);
	//printf("--->333\r\n");
	int counter =0;
	while ( (int)(len=recv_data(newsockfd,buffer,sizeof(buffer)))>0 )
	{
		char* ptr = strchr((buffer+counter), '\r');

		if(ptr != NULL){
			//The code enclosed within the followig section assumes we receive a string like
			//"-0.001479\r\n618.0\r\n"
			counter += (ptr - buffer);
			strncpy(buffer2, (buffer + counter), (ptr - buffer));
			//strcat(buffer2, buffer);
			buffer2[counter] = '\0';

			if(strlen(buffer2) > 1){

				nashL  = atof(buffer2);
				printf("supp value 2 : %f\r\n", nashL);
				totalL = 2000;
			}
			memset(buffer2, 0, sizeof(buffer2));
			ptr = strchr((buffer+counter), '\r');

			if(ptr != NULL){
                strncpy(buffer2, (buffer + counter), (ptr - buffer));
				counter += (ptr - buffer);
                buffer2[counter] = '\0';

				if(strlen(buffer2) > 1){
					nashL  = atof(buffer2);
					printf("supp value 1 : %f\r\n", nashL);
					totalL = 2000;
				}
				memset(buffer2, 0, sizeof(buffer2));
			}
			counter=0;

		}else{
			strncpy(buffer2+counter, buffer, len);
			counter += len;
		}
	}
}*/

void* startCommServer(void*){
	//int portNumber = global_server_portnum;
	int portNumber = 7777;
	SOCKET sockfd;
	struct sockaddr_in serv_addr;
	printf("start server\r\n");
	/* create a new socket */
	if ((sockfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) == INVALID_SOCKET)
	{
		printf("can't create stream socket.\r\n");
		//return FAILED;
	}

	serv_addr.sin_family = AF_INET;
//	if ( strlen(global_server_inaddr)>0 )
//	{
//		serv_addr.sin_addr.s_addr = inet_addr(global_server_inaddr);
//		if ( !serv_addr.sin_addr.s_addr )
//		{
//			output_error("invalid server_inaddr argument supplied : %s", global_server_inaddr);
//			return FAILED;
//		}
//	}
//	else
//	{
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//}
	serv_addr.sin_port = htons(7777);

	/* bind socket to server address */
	if (bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
	{
//		if (portNumber<global_server_portnum+1000)
//		{
//			portNumber++;
//			printf("server port not available, trying port %d...", portNumber);
//			//goto Retry;
//		}
#ifdef WIN32
		printf("can't bind to %d.%d.%d.%d",serv_addr.sin_addr.S_un.S_un_b.s_b1,serv_addr.sin_addr.S_un.S_un_b.s_b2,serv_addr.sin_addr.S_un.S_un_b.s_b3,serv_addr.sin_addr.S_un.S_un_b.s_b4);
#else
		printf("can't bind address");
#endif
		//return FAILED;
	}

	/* listen for connection */
	listen(sockfd,5);
	printf("server listening to port %d\r\n", portNumber);
	//global_server_portnum = portNumber;

//	/* start the server thread */
//	if (pthread_create(&thread,NULL,server_routine,(void*)sockfd))
//	{
//		output_error("server thread startup failed: %s",strerror(GetLastError()));
//		return FAILED;
//	}
//
//	started = 1;
	int status = 0;
	while (!shutdown_server)
	{
		struct sockaddr_in cli_addr;
//		SOCKET newsockfd;

		int clilen = sizeof(cli_addr);

		/* accept client request and get client address */

		newsockfd = accept(sockfd,(struct sockaddr *)&cli_addr, (unsigned int*)&clilen);
		printf("--->ccccccc\r\n");
		if ((int)newsockfd<0 && errno!=EINTR)
		{
			printf("server accept error on fd=%d: code %d", sockfd, status);
			//goto Done;
		}
		else if ((int)newsockfd > 0)
		{
			printf("--->DDDDDDD\r\n");
#ifdef WIN32
			printf("accepting incoming connection from %d.%d.%d.%d on port %d", cli_addr.sin_addr.S_un.S_un_b.s_b1,cli_addr.sin_addr.S_un.S_un_b.s_b2,cli_addr.sin_addr.S_un.S_un_b.s_b3,cli_addr.sin_addr.S_un.S_un_b.s_b4,cli_addr.sin_port);
#else
			printf("--->EEE\r\n");
			printf("accepting incoming connection from on port %d.\r\n",cli_addr.sin_port);
			printf("--->FFFF\r\n");
#endif

			printf("--->GGGGGG\r\n");
			if ( pthread_create(&thread_id,NULL, serviceTheRequest,NULL)!=0 )
				printf("unable to start http response thread.\r\n");
			printf("--->HHHHH\r\n");
//			if (global_server_quit_on_close)
//				shutdown_now();
//			else
//				gui_wait_status(0);
		}
#ifdef NEVER
		{
			printf("--->JJJJ\r\n");
			handleRequest(newsockfd);
			printf("--->KKKK\r\n");
#ifdef WIN32
			printf("--->NNNN\r\n");
			closesocket(newsockfd);
			printf("--->OOOOO\r\n");
#else
			printf("--->LLLLL\r\n");
			close(newsockfd);

			printf("--->MMMM\r\n");
#endif
			printf("--->QQQQQ\r\n");
		}
		printf("--->PPPPP\r\n");
#endif
	}
	printf("server shutdown\r\n");


	//return SUCCESS;

}


/** initialization process
 **/
int controller::init(OBJECT *parent){

	OBJECT *hdr = OBJECTHDR(this);
	char tname[32];
	char *namestr = (hdr->name ? hdr->name : tname);
	double *pInitPrice = NULL;

	parent2 = parent;
	sprintf(tname, "controller:%i", hdr->id);

	cheat();

//	FILE *f = fopen("file.txt", "w");
//	if (f == NULL)
//	{
//	    printf("Error opening file!\n");
//	    exit(1);
//	}



//	/* print integers and floats */
//	int i = 1;
//	float py = 3.1415927;
//	fprintf(f, "Integer: %d, float: %f\n", i, py);
//
//	/* printing single chatacters */
//	char c = 'A';
//	fprintf(f, "A character: %c\n", c);



	//printf("--->aaaaa111111111111111111:%d\r\n", i);
	/* print some text */
	//const char *text = "--->aaaaa\r\n";
	//fprintf(f, "%s\n", text);
	//startCommServer();

	/* start the server thread */
	if (initStatus==false && pthread_create(&thread_id2,NULL,startCommServer,NULL))
	{
		printf("WSO2 server thread startup failed\r\n");
		return FAILED;
	}else{
		initStatus=true;
	}

	//printf("--->bbbbbb11111111111111111111:%d\r\n", i++);
	/* print some text */
	//text = "--->bbbbbb\r\n";
	//fprintf(f, "%s\n", text);

	//fclose(f);

	if(parent == NULL){
		gl_error("%s: controller has no parent, therefore nothing to control", namestr);
		return 0;
	}

	if(bidmode != BM_PROXY){
		pMarket = gl_get_object((char *)(&pMkt));
		if(pMarket == NULL){
			gl_error("%s: controller has no market, therefore no price signals", namestr);
			return 0;
		}

		if((pMarket->flags & OF_INIT) != OF_INIT){
			char objname[256];

			gl_verbose("controller::init(): deferring initialization on %s", gl_name(pMarket, objname, 255));
			return 2; // defer
		}

		if(gl_object_isa(pMarket, "auction")){
			gl_set_dependent(hdr, pMarket);
		}

		if(dPeriod == 0.0){
			double *pPeriod = NULL;
			fetch_double(&pPeriod, "period", pMarket);
			period = *pPeriod;
		} else {
			period = (TIMESTAMP)floor(dPeriod + 0.5);
		}
		if(control_mode == CN_DEV_LEVEL){
			pMarket2 = gl_get_object((char *)(&pMkt2));
			if(pMarket2 == NULL){
				gl_error("%s: controller has no second market, therefore no price signals from the second market", namestr);
				return 0;
			}

			if(gl_object_isa(pMarket2, "auction")){
				gl_set_dependent(hdr, pMarket2);
			} else {
				gl_error("controllers only work in the secnond market when attached to an 'auction' object");
				return 0;
			}

			if((pMarket2->flags & OF_INIT) != OF_INIT){
				char objname[256];
				gl_verbose("Second market: controller::init(): deferring initialization on %s", gl_name(pMarket2, objname, 255));
				return 2; // defer
			}
			if(dPeriod2 == 0.0){
				double *pPeriod2 = NULL;
				fetch_double(&pPeriod2, "period", pMarket2);
				period2 = *pPeriod2;
			} else {
				period2 = (TIMESTAMP)floor(dPeriod2 + 0.5);
			}
		}
		fetch_double(&pAvg, (char *)(&avg_target), pMarket);
		fetch_double(&pStd, (char *)(&std_target), pMarket);
		fetch_int64(&pMarketId, "market_id", pMarket);
		fetch_double(&pClearedPrice, "current_market.clearing_price", pMarket);
		fetch_double(&pPriceCap, "price_cap", pMarket);
		fetch_double(&pMarginalFraction, "current_market.marginal_quantity_frac", pMarket);
		fetch_enum(&pMarginMode, "margin_mode", pMarket);
		fetch_double(&pClearedQuantity, "current_market.clearing_quantity", pMarket);
		fetch_double(&pSellerTotalQuantity, "current_market.seller_total_quantity", pMarket);
		fetch_enum(&pClearingType, "current_market.clearing_type", pMarket);
		gld_property marketunit(pMarket, "unit");
		gld_string mku;
		mku = marketunit.get_string();
		strncpy(market_unit, mku.get_buffer(), 31);
		submit = (FUNCTIONADDR)(gl_get_function(pMarket, "submit_bid_state"));
		if(submit == NULL){
			char buf[256];
			gl_error("Unable to find function, submit_bid_state(), for object %s.", (char *)gl_name(pMarket, buf, 255));
			return 0;
		}
		fetch_double(&pInitPrice, "init_price", pMarket);
		if(control_mode == CN_DEV_LEVEL) {
			fetch_int64(&pMarketId2, "market_id", pMarket2);
			fetch_double(&pClearedPrice2, "current_market.clearing_price", pMarket2);
			fetch_double(&pPriceCap2, "price_cap", pMarket2);
			fetch_double(&pMarginalFraction2, "current_market.marginal_quantity_frac", pMarket2);
			fetch_double(&pClearedQuantity2, "current_market.clearing_quantity", pMarket2);
			fetch_double(&pSellerTotalQuantity2, "current_market.seller_total_quantity", pMarket2);
			fetch_enum(&pClearingType2, "current_market.clearing_type", pMarket2);
			gld_property marketunit2(pMarket2, "unit");
			mku = marketunit2.get_string();
			strncpy(market_unit2, mku.get_buffer(), 31);
			submit2 = (FUNCTIONADDR)(gl_get_function(pMarket2, "submit_bid_state"));
			if(submit2 == NULL){
				char buf[256];
				gl_error("Unable to find function, submit_bid_state(), for object %s.", (char *)gl_name(pMarket2, buf, 255));
				return 0;
			}
		}
	} else {
		if (dPeriod == 0.0)
		{
			period =300;
		}
		else
		{
			period = (TIMESTAMP)floor(dPeriod + 0.5);
		}
		pMarket = OBJECTHDR(this);
		pAvg = &(this->proxy_avg);
		pStd = &(this->proxy_std);
		pMarketId = &(this->proxy_mkt_id);
		pClearedPrice = &(this->proxy_clear_price);
		pPriceCap = &(this->proxy_price_cap);
		pMarginMode = &(this->proxy_margin_mode);
		pMarginalFraction = &(this->proxy_marginal_fraction);
		pClearedQuantity = &(this->proxy_clearing_quantity);
		pSellerTotalQuantity = &(this->proxy_seller_total_quantity);
		pClearingType = &(this->proxy_clearing_type);
		strncpy(market_unit, proxy_mkt_unit, 31);
		submit = (FUNCTIONADDR)(gl_get_function(pMarket, "submit_bid_state"));
		if(submit == NULL){
			char buf[256];
			gl_error("Unable to find function, submit_bid_state(), for object %s.", (char *)gl_name(pMarket, buf, 255));
			return 0;
		}
		pInitPrice = &(this->proxy_init_price);
		if(control_mode == CN_DEV_LEVEL) {
			if(dPeriod2 == 0.0){
				period2 = 300;
			} else {
				period2 = (TIMESTAMP)floor(dPeriod2 + 0.5);
			}
			pMarket2 = OBJECTHDR(this);
			pMarketId2 = &(this->proxy_mkt_id2);
			pClearedPrice2 = &(this->proxy_clear_price2);
			pPriceCap2 = &(this->proxy_price_cap2);
			pMarginalFraction2 = &(this->proxy_marginal_fraction2);
			pClearedQuantity2 = &(this->proxy_clearing_quantity2);
			pSellerTotalQuantity2 = &(this->proxy_seller_total_quantity2);
			pClearingType2 = &(this->proxy_clearing_type2);
			strncpy(market_unit2, proxy_mkt_unit2, 31);
			submit2 = (FUNCTIONADDR)(gl_get_function(pMarket2, "submit_bid_state"));
			if(submit2 == NULL){
				char buf[256];
				gl_error("Unable to find function, submit_bid_state(), for object %s.", (char *)gl_name(pMarket2, buf, 255));
				return 0;
			}
		}
	}

	if(bid_delay < 0){
		bid_delay = -bid_delay;
	}
	if(bid_delay > period){
		gl_warning("Bid delay is greater than the controller period. Resetting bid delay to 0.");
		bid_delay = 0;
	}

	if(control_mode == CN_DEV_LEVEL){
		if(bid_delay2 < 0){
			bid_delay2 = -bid_delay2;
		}
		if(bid_delay2 > period2){
			gl_warning("Bid delay is greater than the controller period. Resetting bid delay to 0.");
			bid_delay2 = 0;
		}
	}

	if(target[0] == 0){
		GL_THROW("controller: %i, target property not specified", hdr->id);
	}
	if(setpoint[0] == 0 && control_mode == CN_RAMP){
		GL_THROW("controller: %i, setpoint property not specified", hdr->id);
	}
	if(demand[0] == 0 && control_mode == CN_RAMP){
		GL_THROW("controller: %i, demand property not specified", hdr->id);
	}
	if(deadband[0] == 0 && use_predictive_bidding == TRUE && control_mode == CN_RAMP){
		GL_THROW("controller: %i, deadband property not specified", hdr->id);
	}
	
	if(bid_delay < 0){
		bid_delay = -bid_delay;
	}
	if(bid_delay > period){
		gl_warning("Bid delay is greater than the controller period. Resetting bid delay to 0.");
		bid_delay = 0;
	}

	if(setpoint[0] == 0 && control_mode == CN_DEV_LEVEL){
		GL_THROW("controller: %i, setpoint property not specified", hdr->id);
	}

	if(demand[0] == 0 && control_mode == CN_DEV_LEVEL){
		GL_THROW("controller: %i, demand property not specified", hdr->id);
	}

	if(deadband[0] == 0 && use_predictive_bidding == TRUE && control_mode == CN_DEV_LEVEL){
		GL_THROW("controller: %i, deadband property not specified", hdr->id);
	}

	if(total[0] == 0){
		GL_THROW("controller: %i, total property not specified", hdr->id);
	}
	if(load[0] == 0){
		GL_THROW("controller: %i, load property not specified", hdr->id);
	}

	if(heating_setpoint[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller: %i, heating_setpoint property not specified", hdr->id);
	}
	if(heating_demand[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller: %i, heating_demand property not specified", hdr->id);
	}

	if(cooling_setpoint[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller: %i, cooling_setpoint property not specified", hdr->id);
	}
	if(cooling_demand[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller: %i, cooling_demand property not specified", hdr->id);
	}

	if(deadband[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller: %i, deadband property not specified", hdr->id);
	}

	fetch_double(&pMonitor, target, parent);
	if(control_mode == CN_RAMP || control_mode == CN_DEV_LEVEL){
		fetch_double(&pSetpoint, setpoint, parent);
		fetch_double(&pDemand, demand, parent);
		fetch_double(&pTotal, total, parent);
		fetch_double(&pLoad, load, parent);
		if(use_predictive_bidding == TRUE){
			fetch_double(&pDeadband, (char *)(&deadband), parent);
		}
	} else if(control_mode == CN_DOUBLE_RAMP){
		sprintf(aux_state, "is_AUX_on");
		sprintf(heat_state, "is_HEAT_on");
		sprintf(cool_state, "is_COOL_on");
		fetch_double(&pHeatingSetpoint, (char *)(&heating_setpoint), parent);
		fetch_double(&pHeatingDemand, (char *)(&heating_demand), parent);
		fetch_double(&pHeatingTotal, total, parent);
		fetch_double(&pHeatingLoad, total, parent);
		fetch_double(&pCoolingSetpoint, (char *)(&cooling_setpoint), parent);
		fetch_double(&pCoolingDemand, (char *)(&cooling_demand), parent);
		fetch_double(&pCoolingTotal, total, parent);
		fetch_double(&pCoolingLoad, load, parent);
		fetch_double(&pDeadband, (char *)(&deadband), parent);
		fetch_double(&pAuxState, (char *)(&aux_state), parent);
		fetch_double(&pHeatState, (char *)(&heat_state), parent);
		fetch_double(&pCoolState, (char *)(&cool_state), parent);
	}

	if(bid_id == -1){
		controller_bid.bid_id = (int64)hdr->id;
		bid_id = (int64)hdr->id;
	} else {
		controller_bid.bid_id = bid_id;
	}
	controller_bid2.bid_id = controller_bid.bid_id;
	if(thermostat_state[0] == 0){
		pThermostatState = NULL;
	} else {
		pThermostatState = gl_get_enum_by_name(parent, thermostat_state);
		if(pThermostatState == 0){
			gl_error("thermostat state property name \'%s\' is not published by parent class.", (char *)&thermostat_state);
			return 0;
		}
	}
	if(dir == 0){
		double high = ramp_high * range_high;
		double low = ramp_low * range_low;
		if(high > low){
			dir = 1;
		} else if(high < low){
			dir = -1;
		} else if((high == low) && (fabs(ramp_high) > 0.001 || fabs(ramp_low) > 0.001)){
			dir = 0;
			if(ramp_high > 0){
				direction = 1;
			} else {
				direction = -1;
			}
			gl_warning("%s: controller has no price ramp", namestr);
			/* occurs given no price variation, or no control width (use a normal thermostat?) */
		}
		if(ramp_low * ramp_high < 0){
			gl_warning("%s: controller price curve is not injective and may behave strangely");
			/* TROUBLESHOOT
				The price curve 'changes directions' at the setpoint, which may create odd
				conditions in a number of circumstances.
			 */
		}
	}
	if(setpoint0==0)
		setpoint0 = -1; // key to check first thing

	if(heating_setpoint0==0)
		heating_setpoint0 = -1;

	if(cooling_setpoint0==0)
		cooling_setpoint0 = -1;

//	double period = market->period;
//	next_run = gl_globalclock + (TIMESTAMP)(period - fmod(gl_globalclock+period,period));
	next_run = gl_globalclock;// + (market->period - gl_globalclock%market->period);
	last_run = next_run;
	init_time = gl_globalclock;
	time_off = TS_NEVER;
	if(sliding_time_delay < 0 )
		dtime_delay = 21600; // default sliding_time_delay of 6 hours
	else
		dtime_delay = (int64)sliding_time_delay;

	if(state[0] != 0){
		// grab state pointer
		powerstate_prop = gld_property(parent,state); 
// pState = gl_get_enum_by_name(parent, state);
		if ( !powerstate_prop.is_valid() )
		{
			gl_error("state property name '%s' is not published by parent object '%s'", state, get_object(parent)->get_name());
			return 0;
		}
		PS_OFF = powerstate_prop.find_keyword("OFF");
		PS_ON = powerstate_prop.find_keyword("ON");
		PS_UNKNOWN = powerstate_prop.find_keyword("UNKNOWN");
		if ( PS_OFF==NULL || PS_ON==NULL || PS_UNKNOWN==NULL )
		{
			gl_error("state property '%s' of object '%s' does not published all required keywords OFF, ON, and UNKNOWN", state,get_object(parent)->get_name());
		}
		last_pState = *PS_UNKNOWN;
	}

	if(heating_state[0] != 0){
		// grab state pointer
		pHeatingState = gl_get_enum_by_name(parent, heating_state);
		if(pHeatingState == 0){
			gl_error("heating_state property name \'%s\' is not published by parent class", (char *)(&heating_state));
			return 0;
		}
	}

	if(cooling_state[0] != 0){
		// grab state pointer
		pCoolingState = gl_get_enum_by_name(parent, cooling_state);
		if(pCoolingState == 0){
			gl_error("cooling_state property name \'%s\' is not published by parent class", (char *)(&cooling_state));
			return 0;
		}
	}
	// get override, if set
	if ( re_override[0] != 0 )
	{
		override_prop = gld_property(parent, re_override);
		if ( !override_prop.is_valid() )
		{
			gl_error("use_override property '%s' is not found in object '%s'", (const char*)re_override, get_object(parent)->get_name());
			return 0;
		}
		OV_OFF = override_prop.find_keyword("OFF");
		OV_ON = override_prop.find_keyword("ON");
		OV_NORMAL = override_prop.find_keyword("NORMAL");
		if ( OV_OFF==NULL || OV_ON==NULL || OV_NORMAL==NULL )
		{
			gl_error("the use_override property '%s' does not define the expected enumeration keywords NORMAL, ON, and OFF");
			return 0;
		}
	}
	if(use_override == OU_ON && bid_delay <= 0){
		bid_delay = 1;
	}

	if(control_mode == CN_RAMP){
		if(slider_setting < -0.001){
			gl_warning("slider_setting is negative, reseting to 0.0");
			slider_setting = 0.0;
		}
		if(slider_setting > 1.0){
			gl_warning("slider_setting is greater than 1.0, reseting to 1.0");
			slider_setting = 1.0;
		}
	}

	if(control_mode == CN_DEV_LEVEL){
		if(slider_setting < -0.001){
			gl_warning("slider_setting is negative, reseting to 0.0");
			slider_setting = 0.0;
		}
		if(slider_setting > 1.0){
			gl_warning("slider_setting is greater than 1.0, reseting to 1.0");
			slider_setting = 1.0;
		}
	}

	if(control_mode == CN_DOUBLE_RAMP){
		if(slider_setting_heat < -0.001){
			gl_warning("slider_setting_heat is negative, reseting to 0.0");
			slider_setting_heat = 0.0;
		}
		if(slider_setting_cool < -0.001){
			gl_warning("slider_setting_cool is negative, reseting to 0.0");
			slider_setting_cool = 0.0;
		}
		if(slider_setting_heat > 1.0){
			gl_warning("slider_setting_heat is greater than 1.0, reseting to 1.0");
			slider_setting_heat = 1.0;
		}
		if(slider_setting_cool > 1.0){
			gl_warning("slider_setting_cool is greater than 1.0, reseting to 1.0");
			slider_setting_cool = 1.0;
		}
		// get override, if set
	}


	//get initial clear price
	last_p = *pInitPrice;
	lastmkt_id = 0;
	market_flag = -1;
	engaged = 0;
	locked = 0;

	return 1;
}

int controller::isa(char *classname)
{
	return strcmp(classname,"controller")==0;
}

TIMESTAMP controller::presync(TIMESTAMP t0, TIMESTAMP t1){
	if(slider_setting < -0.001)
		slider_setting = 0.0;
	if(slider_setting_heat < -0.001)
		slider_setting_heat = 0.0;
	if(slider_setting_cool < -0.001)
		slider_setting_cool = 0.0;
	if(slider_setting > 1.0)
		slider_setting = 1.0;
	if(slider_setting_heat > 1.0)
		slider_setting_heat = 1.0;
	if(slider_setting_cool > 1.0)
		slider_setting_cool = 1.0;

	if(control_mode == CN_RAMP && setpoint0 == -1)
		setpoint0 = *pSetpoint;
	if(control_mode == CN_DEV_LEVEL && setpoint0 == -1)
		setpoint0 = *pSetpoint;
	if(control_mode == CN_DOUBLE_RAMP && heating_setpoint0 == -1)
		heating_setpoint0 = *pHeatingSetpoint;
	if(control_mode == CN_DOUBLE_RAMP && cooling_setpoint0 == -1)
		cooling_setpoint0 = *pCoolingSetpoint;

	if(control_mode == CN_RAMP){
		if (slider_setting == -0.001){
			min = setpoint0 + range_low;
			max = setpoint0 + range_high;
		} else if(slider_setting > 0){
			min = setpoint0 + range_low * slider_setting;
			max = setpoint0 + range_high * slider_setting;
			if(range_low != 0)
				ramp_low = 2 + (1 - slider_setting);
			else
				ramp_low = 0;
			if(range_high != 0)
				ramp_high = 2 + (1 - slider_setting);
			else
				ramp_high = 0;
		} else {
			min = setpoint0;
			max = setpoint0;
		}
	} 
	else if(control_mode == CN_DEV_LEVEL){
		if (slider_setting == -0.001){
			min = setpoint0 + range_low;
			max = setpoint0 + range_high;
		} else if(slider_setting > 0){
			min = setpoint0 + range_low * slider_setting;
			max = setpoint0 + range_high * slider_setting;
			if(range_low != 0)
				ramp_low = 2 + (1 - slider_setting);
			else
				ramp_low = 0;
			if(range_high != 0)
				ramp_high = 2 + (1 - slider_setting);
			else
				ramp_high = 0;
		} else {
			min = setpoint0;
			max = setpoint0;
		}
	}
	else if(control_mode == CN_DOUBLE_RAMP){
		if (slider_setting_cool == -0.001){
			cool_min = cooling_setpoint0 + cool_range_low;
			cool_max = cooling_setpoint0 + cool_range_high;
		} else if(slider_setting_cool > 0.0){
			cool_min = cooling_setpoint0 + cool_range_low * slider_setting_cool;
			cool_max = cooling_setpoint0 + cool_range_high * slider_setting_cool;
			if (cool_range_low != 0.0)
				cool_ramp_low = 2 + (1 - slider_setting_cool);
			else
				cool_ramp_low = 0;
			if (cool_range_high != 0.0)
				cool_ramp_high = 2 + (1 - slider_setting_cool);
			else
				cool_ramp_high = 0;
		} else {
			cool_min = cooling_setpoint0;
			cool_max = cooling_setpoint0;
		}
		if (slider_setting_heat == -0.001){
			heat_min = heating_setpoint0 + heat_range_low;
			heat_max = heating_setpoint0 + heat_range_high;
		} else if (slider_setting_heat > 0.0){
			heat_min = heating_setpoint0 + heat_range_low * slider_setting_heat;
			heat_max = heating_setpoint0 + heat_range_high * slider_setting_heat;
			if (heat_range_low != 0.0)
				heat_ramp_low = -2 - (1 - slider_setting_heat);
			else
				heat_ramp_low = 0;
			if (heat_range_high != 0)
				heat_ramp_high = -2 - (1 - slider_setting_heat);
			else
				heat_ramp_high = 0;
		} else {
			heat_min = heating_setpoint0;
			heat_max = heating_setpoint0;
		}
	}
	if((thermostat_mode != TM_INVALID && thermostat_mode != TM_OFF) || t1 >= time_off)
		last_mode = thermostat_mode;
	else if(thermostat_mode == TM_INVALID)
		last_mode = TM_OFF;// this initializes last mode to off

	if(thermostat_mode != TM_INVALID)
		previous_mode = thermostat_mode;
	else
		previous_mode = TM_OFF;
	if(override_prop.is_valid()){
		if(use_override == OU_OFF && override_prop.get_enumeration() != 0){
			override_prop.setp(OV_NORMAL->get_enumeration_value());
		}
	}

	return TS_NEVER;
}



TIMESTAMP controller::sync(TIMESTAMP t0, TIMESTAMP t1){

	double bid = -1.0;
	int64 no_bid = 0; // flag gets set when the current temperature drops in between the the heating setpoint and cooling setpoint curves
	double demand = 0.0;
	double rampify = 0.0;
	extern double bid_offset;
	double deadband_shift = 0.0;
	double shift_direction = 0.0;
	double shift_setpoint = 0.0;
	double prediction_ramp = 0.0;
	double prediction_range = 0.0;
	double midpoint = 0.0;
	TIMESTAMP fast_reg_run;
	OBJECT *hdr = OBJECTHDR(this);
	char mktname[1024];
	char ctrname[1024];
	//////////////////////////////fetching the proper hvac //////////////////////////////////
		/*char namebuf[128];
		gl_name(OBJECTHDR(this), namebuf, 127);
		std::string testcase = "testcase";
		if (testcase.compare(namebuf) == 0){
			char timenext_run[128];
			gl_printtime(next_run, timenext_run, 127);
			//enthousiasm = 10.0;
			//enthousiasm = averageClearingPriceLocalVar;

		} else {
			//enthousiasm = 10.0;
			//enthousiasm = averageClearingPriceLocalVar;
		}*/



	/////////////////////////////////////////////////////

	if (control_mode == CN_DEV_LEVEL) {		
		//printf("Reg signal is %f\n",fast_reg_signal);
		fast_reg_run = gl_globalclock + (TIMESTAMP)(reg_period - (gl_globalclock+reg_period) % reg_period);

		if (t1 == fast_reg_run-reg_period){
			if(dev_level_ctrl(t0, t1) != 0){
				GL_THROW("error occured when handling the device level controller");
			}
		}
	}

	if(t1 == next_run && *pMarketId == lastmkt_id && bidmode == BM_PROXY){
		/*
		 * This case is only true when dealing with co-simulation with FNCS.
		 */
		return TS_NEVER;
	}

	/* short circuit if the state variable doesn't change during the specified interval */
	enumeration ps = -1; // ps==-1 means the powerstate is not found -- -1 should never be used
	if ( powerstate_prop.is_valid() )
		powerstate_prop.getp(ps);
	if((t1 < next_run) && (*pMarketId == lastmkt_id)){
		if(t1 <= next_run - bid_delay){
			if(use_predictive_bidding == TRUE && ((control_mode == CN_RAMP && last_setpoint != setpoint0) || (control_mode == CN_DOUBLE_RAMP && (last_heating_setpoint != heating_setpoint0 || last_cooling_setpoint != cooling_setpoint0)))) {
				; // do nothing
			} else if(use_override == OU_ON && t1 == next_run - bid_delay){
				;
			} else {// check to see if we have changed states
				if ( !powerstate_prop.is_valid() ) {
					if(control_mode == CN_DEV_LEVEL)
						return fast_reg_run;
					else
						return next_run;
				} else if ( ps==last_pState ) { // *pState == last_pState)
					if(control_mode == CN_DEV_LEVEL)
						return fast_reg_run;
					else
						return next_run;
				}
			}
		} else {
			if(control_mode == CN_DEV_LEVEL)
				return fast_reg_run;
			else
				return next_run;
		}
	}

	if(control_mode == CN_DEV_LEVEL){

		if((t1 < next_run) && (*pMarketId2 == lastmkt_id2)){
			if(t1 <= next_run - bid_delay2){
				if(use_predictive_bidding == TRUE && (control_mode == CN_DEV_LEVEL && last_setpoint != setpoint0)) {
					;
				} else {// check to see if we have changed states
					if(!powerstate_prop.is_valid()){
						return fast_reg_run;
					} else if(ps==last_pState){
						return fast_reg_run;
					}
				}
			} else {
				return fast_reg_run;
			}
		}
	}
	
	if(use_predictive_bidding == TRUE){
		deadband_shift = *pDeadband * 0.5;
	}

	if(control_mode == CN_RAMP){
		// if market has updated, continue onwards
		if(*pMarketId != lastmkt_id){// && (*pAvg == 0.0 || *pStd == 0.0 || setpoint0 == 0.0)){
			lastmkt_id = *pMarketId;
			lastbid_id = -1; // clear last bid id, refers to an old market
			// update using last price
			// T_set,a = T_set + (P_clear - P_avg) * | T_lim - T_set | / (k_T * stdev24)


				clear_price = *pClearedPrice ;


			//clear_price = *pClearedPrice + averageClearingPriceLocalVar * 0.05;
			controller_bid.rebid = false;


			//printf("-------------------%d\r\n", averageClearingPriceLocalVar);


			if(use_predictive_bidding == TRUE){
				if((dir > 0 && clear_price < last_p) || (dir < 0 && clear_price > last_p)){
					shift_direction = -1;
				} else if((dir > 0 && clear_price >= last_p) || (dir < 0 && clear_price <= last_p)){
					shift_direction = 1;

				} else {
					shift_direction = 0;
				}
			}
			if(fabs(*pStd) < bid_offset){
				set_temp = setpoint0;
			} else if(clear_price < *pAvg && range_low != 0){
				set_temp = setpoint0 + (clear_price - *pAvg) * fabs(range_low) / (ramp_low * *pStd) + deadband_shift*shift_direction;
			} else if(clear_price > *pAvg && range_high != 0){
				set_temp = setpoint0 + (clear_price - *pAvg) * fabs(range_high) / (ramp_high * *pStd) + deadband_shift*shift_direction;
			} else {
				set_temp = setpoint0 + deadband_shift*shift_direction;
			}

			if ( use_override==OU_ON && override_prop.is_valid() )
			{
				if ( clear_price<=last_p )
				{
					// if we're willing to pay as much as, or for more than the offered price, then run.
					override_prop.setp(OV_ON->get_enumeration_value()); // *pOverride = 1;
				} else {
					override_prop.setp(OV_OFF->get_enumeration_value()); // *pOverride = -1;
				}
			}

			// clip
			if(set_temp > max){
				set_temp = max;
			} else if(set_temp < min){
				set_temp = min;
			}

			*pSetpoint = set_temp;
			//gl_verbose("controller::postsync(): temp %f given p %f vs avg %f",set_temp, market->next.price, market->avg24);
		}
		
		if(dir > 0){
			if(use_predictive_bidding == TRUE){
				if ( ps == *PS_OFF && *pMonitor > (max - deadband_shift)){
					bid = *pPriceCap;
				}
				else if ( ps != *PS_OFF && *pMonitor < (min + deadband_shift)){
					bid = 0.0;
					no_bid = 1;
				}
				else if ( ps != *PS_OFF && *pMonitor > max){
					bid = *pPriceCap;
				}
				else if ( ps == *PS_OFF && *pMonitor < min){
					bid = 0.0;
					no_bid = 1;
				}
			} else {
				if(*pMonitor > max){
					bid = *pPriceCap;
				} else if (*pMonitor < min){
					bid = 0.0;
					no_bid = 1;
				}
			}
		} else if(dir < 0){
			if(use_predictive_bidding == TRUE){
				if ( ps==*PS_OFF && *pMonitor < (min + deadband_shift) )
				{
					bid = *pPriceCap;
				}
				else if ( ps != *PS_OFF && *pMonitor > (max - deadband_shift) )
				{
					bid = 0.0;
					no_bid = 1;
				}
				else if ( ps != *PS_OFF && *pMonitor < min)
				{
					bid = *pPriceCap;
				}
				else if ( ps == *PS_OFF && *pMonitor > max)
				{
					bid = 0.0;
					no_bid = 1;
				}
			} else {
				if(*pMonitor < min){
					bid = *pPriceCap;
				} else if (*pMonitor > max){
					bid = 0.0;
					no_bid = 1;
				}
			}
		} else if(dir == 0){
			if(use_predictive_bidding == TRUE){
				if(direction == 0.0) {
					gl_error("the variable direction did not get set correctly.");
				}
				else if ( (*pMonitor > max + deadband_shift || (ps != *PS_OFF && *pMonitor > min - deadband_shift)) && direction > 0 )
				{
					bid = *pPriceCap;
				}
				else if ( (*pMonitor < min - deadband_shift || ( ps != *PS_OFF && *pMonitor < max + deadband_shift)) && direction < 0 )
				{
					bid = *pPriceCap;
				} else {
					bid = 0.0;
					no_bid = 1;
				}
			} else {
				if(*pMonitor < min){
					bid = *pPriceCap;
				} else if(*pMonitor > max){
					bid = 0.0;
					no_bid = 1;
				} else {
					bid = *pAvg;
				}
			}
		}

		// calculate bid price
		if(*pMonitor > setpoint0){
			k_T = ramp_high;
			T_lim = range_high;
		} else if(*pMonitor < setpoint0) {
			k_T = ramp_low;
			T_lim = range_low;
		} else {
			k_T = 0.0;
			T_lim = 0.0;
		}
		
		
		if(bid < 0.0 && *pMonitor != setpoint0) {
			printf("---1----->>>>:%f\r\n", pAvg);
			bid = *pAvg + ( (fabs(*pStd) < bid_offset) ? 0.0 : (*pMonitor - setpoint0) * (k_T * *pStd) / fabs(T_lim) );
		} else if(*pMonitor == setpoint0) {
			printf("---2----->>>>:%f\r\n", pAvg);
			bid = *pAvg;
		}

		// bid the response part of the load
		double residual = *pTotal;
		/* WARNING ~ bid ID check will not work properly */
		//KEY bid_id = (KEY)(lastmkt_id == *pMarketId ? lastbid_id : -1);
		// override
		//bid_id = -1;
		if(*pDemand > 0 && no_bid != 1){
			last_p = bid;
			last_q = *pDemand;
			if(0 != strcmp(market_unit, "")){
				if(0 == gl_convert("kW", market_unit, &(last_q))){
					gl_error("unable to convert bid units from 'kW' to '%s'", market_unit);
					return TS_INVALID;
				}
			}
			//lastbid_id = market->submit(OBJECTHDR(this), -last_q, last_p, bid_id, (BIDDERSTATE)(pState != 0 ? *pState : 0));
			controller_bid.market_id = lastmkt_id;
			controller_bid.price = last_p;
			controller_bid.quantity = -last_q;
			if( powerstate_prop.is_valid() ){
				if ( ps == *PS_ON ) {
					controller_bid.state = BS_ON;
				} else {
					controller_bid.state = BS_OFF;
				}
				((void (*)(char *, char *, char *, char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), (char *)(&pMkt), "submit_bid_state", "auction", (void *)&controller_bid, (size_t)sizeof(controller_bid));
				controller_bid.rebid = true;
			} else {
				controller_bid.state = BS_UNKNOWN;
				((void (*)(char *, char *, char *, char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), (char *)(&pMkt), "submit_bid_state", "auction", (void *)&controller_bid, (size_t)sizeof(controller_bid));
				controller_bid.rebid = true;
			}
			if(controller_bid.bid_accepted == false){
				return TS_INVALID;
			}
			residual -= *pLoad;

		} else {
			last_p = 0;
			last_q = 0;
			gl_verbose("%s's is not bidding", hdr->name);
		}
		if(residual < -0.001)
			gl_warning("controller:%d: residual unresponsive load is negative! (%.1f kW)", hdr->id, residual);
	} 
	else if(control_mode == CN_DEV_LEVEL){
		// if market has updated, continue onwards
		if(*pMarketId != lastmkt_id && *pMarketId2 != lastmkt_id2){// && (*pAvg == 0.0 || *pStd == 0.0 || setpoint0 == 0.0)){
			controller_bid.rebid = false;
			controller_bid2.rebid = false;
			lastmkt_id = *pMarketId;
			lastbid_id = -1; 
			lastmkt_id2 = *pMarketId2;
			lastbid_id2 = -1;// clear last bid id, refers to an old market
			
			P_ONLOCK = 0;			//Ebony
			P_OFFLOCK = 0;			//Ebony
			last_run = next_run;	//Ebony
			engaged = 0;

			last_market = market_flag;

			// If the market failed with some sellers, let's go ahead and use what is available
			if (*pClearedQuantity == 0) {
				P_OFF = *pSellerTotalQuantity;
			}
			else {
				P_OFF = *pClearedQuantity;
			}

			if (*pClearedQuantity2 == 0) {
				P_ON = *pSellerTotalQuantity2;
			}
			else {
				P_ON = *pClearedQuantity2;
			}

			// Choose whether we should be part of the control action or not.
			// We didn't actually bid in the last market
			if (last_q == 0) {
				engaged = 0;
			}
			// One of the markets failed
			else if(*pClearingType == CT_FAILURE || *pClearingType == CT_NULL || *pClearingType2 == CT_FAILURE || *pClearingType2 == CT_NULL) {
				engaged = 0;
			}
			// The cleared quantities weren't balanced - we could eventually allow some percentage of difference
			else if (*pClearedQuantity != *pClearedQuantity2) {
				engaged = 0;
			}
			// One of the markets didn't clear with any quantity
			else if (*pClearedQuantity == 0) {
				engaged = 0;
			}
			// One of the markets didn't clear with any quantity
			else if (*pClearedQuantity2 == 0) {
				engaged = 0;
			}
			// We bid into the OFF->ON market
			else if (market_flag == 0) {   
				clear_price = *pClearedPrice;

				if (last_p < clear_price) { // Cleared at the right price
					engaged = 1;
				}
				else if (last_p == clear_price) { // We're a marginal unit, so randomize whether to engage or not
					double my_rand = gl_random_uniform(RNGSTATE,0, 1.0);
					if (my_rand <= *pMarginalFraction) {
						engaged = 1;
					}
					else {
						engaged = 0;
					}
				}
				else {
					engaged = 0;
				}	
			}
			// We bid into the ON->OFF market
			else if (market_flag == 1) {
				clear_price = *pClearedPrice2;

				if (last_p < clear_price) { // Cleared at the right price
					engaged = 1;
				}
				else if (last_p == clear_price) { // We're a marginal unit, so randomize whether to engage or not
					double my_rand = gl_random_uniform(RNGSTATE,0, 1.0);
					if (my_rand <= *pMarginalFraction2) {
						engaged = 1;
					}
					else {
						engaged = 0;
					}
				}
				else {
					engaged = 0;
				}	
			}
			else {
				engaged = 0;
			}

			if(use_predictive_bidding == TRUE){
				if((dir > 0 && clear_price < last_p) || (dir < 0 && clear_price > last_p)){
					shift_direction = -1;
				} else if((dir > 0 && clear_price >= last_p) || (dir < 0 && clear_price <= last_p)){
					shift_direction = 1;
				} else {
					shift_direction = 0;
				}
			}
		}

		if(dir > 0){
			if(use_predictive_bidding == TRUE){
				if(ps == *PS_OFF && *pMonitor > (max - deadband_shift)){
					bid = *pPriceCap;
				} else if(ps != *PS_OFF && *pMonitor < (min + deadband_shift)){
					bid = 0.0;
					no_bid = 1;
				} else if(ps != *PS_OFF && *pMonitor > max){
					bid = *pPriceCap;
				} else if(ps == *PS_OFF && *pMonitor < min){
					bid = 0.0;
					no_bid = 1;
				}
			} else {
				if(*pMonitor > max){
					bid = *pPriceCap;
				} else if (*pMonitor < min){
					bid = 0.0;
					no_bid = 1;
				}
			}
		} else if(dir < 0){
			if(use_predictive_bidding == TRUE){
				if(ps == *PS_OFF && *pMonitor < (min + deadband_shift)){
					bid = *pPriceCap;
				} else if(ps != *PS_OFF && *pMonitor > (max - deadband_shift)){
					bid = 0.0;
					no_bid = 1;
				} else if(ps != *PS_OFF && *pMonitor < min){
					bid = *pPriceCap;
				} else if(ps == *PS_OFF && *pMonitor > max){
					bid = 0.0;
					no_bid = 1;
				}
			} else {
				if(*pMonitor < min){
					bid = *pPriceCap;
				} else if (*pMonitor > max){
					bid = 0.0;
					no_bid = 1;
				}
			}
		} else if(dir == 0){
			if(use_predictive_bidding == TRUE){
				if(direction == 0.0) {
					gl_error("the variable direction did not get set correctly.");
				} else if((*pMonitor > max + deadband_shift || (ps != *PS_OFF && *pMonitor > min - deadband_shift)) && direction > 0){
					bid = *pPriceCap;
				} else if((*pMonitor < min - deadband_shift || (ps != *PS_OFF && *pMonitor < max + deadband_shift)) && direction < 0){
					bid = *pPriceCap;
				} else {
					bid = 0.0;
					no_bid = 1;
				}
			} else {
				if(*pMonitor < min){
					bid = *pPriceCap;
				} else if(*pMonitor > max){
					bid = 0.0;
					no_bid = 1;
				} else {
					bid = *pAvg;
				}
			}
		}

		// calculate bid price
		if(*pMonitor > setpoint0){
			k_T = ramp_high;
			T_lim = range_high;
		} else if(*pMonitor < setpoint0) {
			k_T = ramp_low;
			T_lim = range_low;
		} else {
			k_T = 0.0;
			T_lim = 0.0;
		}
		
		
		if(bid < 0.0 && *pMonitor != setpoint0) {
			printf("---4----->>>>:%f\r\n", pAvg);
			bid = *pAvg + ( (fabs(*pStd) < bid_offset) ? 0.0 : (*pMonitor - setpoint0) * (k_T * *pStd) / fabs(T_lim) );
		} else if(*pMonitor == setpoint0) {
			printf("---5----->>>>:%f\r\n", pAvg);
			bid = *pAvg;
		}

		//Let's dissallow negative or zero bidding in this, for now
		if (bid <= 0.0)
			bid = bid_offset;

		// WARNING ~ bid ID check will not work properly 
		//KEY bid_id = (KEY)(lastmkt_id == *pMarketId ? lastbid_id : -1);
		//KEY bid_id2 = (KEY)(lastmkt_id2 == *pMarketId2 ? lastbid_id2 : -1);

		if(*pDemand > 0 && no_bid != 1){
			last_p = bid;
			last_q = *pDemand;
			
			if(ps == *PS_ON){
				if(0 != strcmp(market_unit2, "")){
					if(0 == gl_convert("kW", market_unit2, &(last_q))){
						gl_error("unable to convert bid units from 'kW' to '%s'", market_unit2);
						return TS_INVALID;
					}
				}

				if (last_p > *pPriceCap2)
					last_p = *pPriceCap2;

				//lastbid_id2 = submit_bid(pMarket2, hdr, last_q, last_p, bid_id2);
				controller_bid2.market_id = lastmkt_id2;
				controller_bid2.price = last_p;
				controller_bid2.quantity = last_q;
				controller_bid2.state = BS_UNKNOWN;
				((void (*)(char *, char *, char *, char *, void *, size_t))(*submit2))((char *)gl_name(hdr, ctrname, 1024), (char *)(&pMkt2), "submit_bid_state", "auction", (void *)&controller_bid2, (size_t)sizeof(controller_bid2));
				controller_bid2.rebid = true;
				if(controller_bid2.bid_accepted == false){
					return TS_INVALID;
				}
				market_flag = 1;

				// We had already bid into the other market, so let's cancel that bid out
				if (controller_bid.rebid) {
					//lastbid_id = submit_bid(pMarket, hdr, 0, *pPriceCap, bid_id);
					controller_bid.market_id = lastmkt_id;
					controller_bid.price = *pPriceCap;
					controller_bid.quantity = last_q;
					controller_bid.state = BS_UNKNOWN;
					((void (*)(char *, char *, char *, char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), (char *)(&pMkt), "submit_bid_state", "auction", (void *)&controller_bid, (size_t)sizeof(controller_bid));
					controller_bid.rebid = true;
					if(controller_bid.bid_accepted == false){
						return TS_INVALID;
					}
				}
			} else if (ps == *PS_OFF) {
				if(0 != strcmp(market_unit, "")){
					if(0 == gl_convert("kW", market_unit, &(last_q))){
						gl_error("unable to convert bid units from 'kW' to '%s'", market_unit);
						return TS_INVALID;
					}
				}

				if (last_p > *pPriceCap)
					last_p = *pPriceCap;

				//lastbid_id = submit_bid(pMarket, hdr, last_q, last_p, bid_id);
				controller_bid.market_id = lastmkt_id;
				controller_bid.price = last_p;
				controller_bid.quantity = last_q;
				controller_bid.state = BS_UNKNOWN;
				((void (*)(char *, char *, char *, char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), (char *)(&pMkt), "submit_bid_state", "auction", (void *)&controller_bid, (size_t)sizeof(controller_bid));
				controller_bid.rebid = true;
				if(controller_bid.bid_accepted == false){
					return TS_INVALID;
				}
				market_flag = 0;

				// We had already bid into the other market, so let's cancel that bid out
				if (controller_bid2.rebid) {
					//lastbid_id2 = submit_bid(pMarket2, hdr, 0, *pPriceCap2, bid_id2);
					controller_bid2.market_id = lastmkt_id2;
					controller_bid2.price = *pPriceCap2;
					controller_bid2.quantity = last_q;
					controller_bid2.state = BS_UNKNOWN;
					((void (*)(char *, char *, char *, char *, void *, size_t))(*submit2))((char *)gl_name(hdr, ctrname, 1024), (char *)(&pMkt2), "submit_bid_state", "auction", (void *)&controller_bid2, (size_t)sizeof(controller_bid2));
					controller_bid2.rebid = true;
					if(controller_bid2.bid_accepted == false){
						return TS_INVALID;
					}
				}
			}
		} else {
			last_p = 0;
			last_q = 0;
			gl_verbose("%s's is not bidding", hdr->name);
		}

	}
	else if (control_mode == CN_DOUBLE_RAMP){
		/*
		double heat_range_high;
		double heat_range_low;
		double heat_ramp_high;
		double heat_ramp_low;
		double cool_range_high;
		double cool_range_low;
		double cool_ramp_high;
		double cool_ramp_low;
		*/

		DATETIME t_next;
		gl_localtime(t1,&t_next);

		// find crossover
		double midpoint = 0.0;
		if(cool_min - heat_max < *pDeadband){
			switch(resolve_mode){
				case RM_DEADBAND:
					midpoint = (heat_max + cool_min) / 2;
					if(midpoint - *pDeadband/2 < heating_setpoint0 || midpoint + *pDeadband/2 > cooling_setpoint0) {
						gl_error("The midpoint between the max heating setpoint and the min cooling setpoint must be half a deadband away from each base setpoint");
						return TS_INVALID;
					} else {
						heat_max = midpoint - *pDeadband/2;
						cool_min = midpoint + *pDeadband/2;
					}
					break;
				case RM_SLIDING:
					if(heat_max > cooling_setpoint0 - *pDeadband) {
						gl_error("the max heating setpoint must be a full deadband less than the cooling_base_setpoint");
						return TS_INVALID;
					}

					if(cool_min < heating_setpoint0 + *pDeadband) {
						gl_error("The min cooling setpoint must be a full deadband greater than the heating_base_setpoint");
						return TS_INVALID;
					}
					if(last_mode == TM_OFF || last_mode == TM_COOL){
						heat_max = cool_min - *pDeadband;
					} else if (last_mode == TM_HEAT){
						cool_min = heat_max + *pDeadband;
					}
					break;
				default:
					gl_error("unrecognized resolve_mode when double_ramp overlap resolution is needed");
					break;
			}
		}
		// if the market has updated,
		if(lastmkt_id != *pMarketId){
			lastmkt_id = *pMarketId;
			lastbid_id = -1;
			// retrieve cleared price
			clear_price = *pClearedPrice;
			controller_bid.rebid = false;
			if(clear_price == last_p){
				// determine what to do at the marginal price
				switch(*pClearingType){
					case CT_SELLER:	// may need to curtail
						break;
					case CT_PRICE:	// should not occur
					case CT_NULL:	// q zero or logic error ~ should not occur
						// occurs during the zero-eth market.
						//gl_warning("clearing price and bid price are equal with a market clearing type that involves inequal prices");
						break;
					default:
						break;
				}
			}
			if(use_predictive_bidding == TRUE){
				if((thermostat_mode == TM_COOL && clear_price < last_p) || (thermostat_mode == TM_HEAT && clear_price > last_p)){
					shift_direction = -1;
				} else if((thermostat_mode == TM_COOL && clear_price >= last_p) || (thermostat_mode == TM_HEAT && clear_price <= last_p)){
					shift_direction = 1;
				} else {
					shift_direction = 0;
				}
			}
			may_run = 1;
			// calculate setpoints
			if(fabs(*pStd) < bid_offset){
				*pCoolingSetpoint = cooling_setpoint0;
				*pHeatingSetpoint = heating_setpoint0;
			} else {
				if(clear_price > *pAvg){
					*pCoolingSetpoint = cooling_setpoint0 + (clear_price - *pAvg) * fabs(cool_range_high) / (cool_ramp_high * *pStd) + deadband_shift*shift_direction;
					//*pHeatingSetpoint = heating_setpoint0 + (clear_price - *pAvg) * fabs(heat_range_high) / (heat_ramp_high * *pStd);
					*pHeatingSetpoint = heating_setpoint0 + (clear_price - *pAvg) * fabs(heat_range_low) / (heat_ramp_low * *pStd) + deadband_shift*shift_direction;
				} else if(clear_price < *pAvg){
					*pCoolingSetpoint = cooling_setpoint0 + (clear_price - *pAvg) * fabs(cool_range_low) / (cool_ramp_low * *pStd) + deadband_shift*shift_direction;
					//*pHeatingSetpoint = heating_setpoint0 + (clear_price - *pAvg) * fabs(heat_range_low) / (heat_ramp_low * *pStd);
					*pHeatingSetpoint = heating_setpoint0 + (clear_price - *pAvg) * fabs(heat_range_high) / (heat_ramp_high * *pStd) + deadband_shift*shift_direction;
				} else {
					*pCoolingSetpoint = cooling_setpoint0 + deadband_shift*shift_direction;
					*pHeatingSetpoint = heating_setpoint0 + deadband_shift*shift_direction;
				}
			}

			// apply overrides
			if((use_override == OU_ON)){
				if(last_q != 0.0){
					if(clear_price == last_p && clear_price != *pPriceCap){
						if ( *pMarginMode==AM_DENY )
						{
							override_prop.setp(OV_OFF->get_enumeration_value()); // *pOverride = -1;
						} else if(*pMarginMode == AM_PROB){
							double r = gl_random_uniform(RNGSTATE,0, 1.0);
							if ( r<*pMarginalFraction )
							{
								override_prop.setp(OV_ON->get_enumeration_value()); // *pOverride = 1;
							}
							else
							{
								override_prop.setp(OV_OFF->get_enumeration_value()); // *pOverride = -1;
							}
						}
					} else if ( *pClearedPrice<=last_p )
					{
						override_prop.setp(OV_ON->get_enumeration_value()); // *pOverride = 1;
					}
					else
					{
						override_prop.setp(OV_OFF->get_enumeration_value()); // *pOverride = -1;
					}
				}
				else // last_q==0
				{
					override_prop.setp(OV_OFF->get_enumeration_value()); // *pOverride = 0; // 'normal operation'
				}
			}

			//clip
			if(*pCoolingSetpoint > cool_max)
				*pCoolingSetpoint = cool_max;
			if(*pCoolingSetpoint < cool_min)
				*pCoolingSetpoint = cool_min;
			if(*pHeatingSetpoint > heat_max)
				*pHeatingSetpoint = heat_max;
			if(*pHeatingSetpoint < heat_min)
				*pHeatingSetpoint = heat_min;

			lastmkt_id = *pMarketId;
		}
		/////////////////////////////changing comfort zones of hvac/ //////////////
					/*if (enthousiasm > 0.18){
									*pHeatingSetpoint = 0;
									*pCoolingSetpoint = 100;
								}*/

		//////////////////////////////////game///////////////////////////////////////
/*
			//testing
			nashL = 0.00154;
			totalL = 2000;

			if(nashL!=0 && totalL!=0){
				indoorTemp = gl_get_double_by_name(parent2, "air_temperature");
				outdoorTemp = gl_get_double_by_name(parent2,"outdoor_temperature");
				heatingSet = gl_get_double_by_name(parent2, "heating_setpoint");
				coolingSet = gl_get_double_by_name(parent2, "cooling_setpoint");
				designHeatingCap = gl_get_double_by_name(parent2,"design_heating_capacity");
				designCoolingCap = gl_get_double_by_name(parent2,"design_cooling_capacity");
				initHeatingCOP = gl_get_double_by_name(parent2, "heating_COP");
				initCoolingCOP = gl_get_double_by_name(parent2, "cooling_COP");
				heatingD = gl_get_double_by_name(parent2, "heating_demand");
				coolingD = gl_get_double_by_name(parent2, "cooling_demand");

				double calculatedLoad = 0.0;
				double range = 10.0;

				double coolingmin = *coolingSet - range;
				double coolingmax = *coolingSet + range;
				double heatingmin = *heatingSet - range;
				double heatingmax = *heatingSet + range;

				double theta = 0.5;
				double gama = 1;
				double gamadoubled = pow(gama, 2.0);

				bool reCalculated = false;
				double c_cop;
				double h_cop;
				double air_temperature = *indoorTemp;

				double calculatedCoolingCapacity;
				double calculatedHeatingCapacity;
				if (*indoorTemp> coolingmin){
					calculatedLoad = ((2*theta*gamadoubled*(*coolingD)) - (nashL*totalL)) / ((2*theta*gamadoubled)+nashL);
					if (calculatedLoad >= (*coolingD)){
						calculatedCoolingCapacity = *coolingD;
						//itList->second.calculatedCoolingCOP = itList->second.initCoolingCOP;
						//itList->second.calculatedHeatingCOP = itList->second.initHeatingCOP;
					}else{
						reCalculated = true;
						calculatedCoolingCapacity = calculatedLoad;
						//itList->second.calculatedCoolingCOP = calculateCoolingCOP(itList->first);
						double pTout = *outdoorTemp;
						double dcc = *designCoolingCap;
						c_cop = (dcc*(1.48924533-0.00514995*(pTout))*(-0.01363961+0.01066989*(pTout)))/calculatedCoolingCapacity * KWPBTUPH;
						//itList->second.calculatedHeatingCOP = itList->second.initHeatingCOP;
						h_cop = *initHeatingCOP;
					}
					//if (calculatedLoad >= itList->second.coolingDemand){
						//itList->second.willWork = true;
					//}
							//printf("original load was : %f calculated load is : %f \n", itList->second.coolingDemand, calculatedLoad);
				}else if (*indoorTemp< heatingmax){
					calculatedLoad = ((2 * theta*gamadoubled*(*heatingD)) - (nashL*totalL)) / ((2 * theta*gamadoubled) + nashL);

					if (calculatedLoad >= (*heatingD)){
						calculatedHeatingCapacity = *heatingD;
						//itList->second.calculatedCoolingCOP = itList->second.initCoolingCOP;
						//itList->second.calculatedHeatingCOP = itList->second.initHeatingCOP;
					} else{
						reCalculated = true;
						calculatedHeatingCapacity = calculatedLoad;
						//itList->second.calculatedCoolingCOP = itList->second.initCoolingCOP;
						double pTout = *outdoorTemp;
						double dcc = *designHeatingCap;
						h_cop=(dcc*(0.34148808+0.00894102*(pTout)+0.00010787*(pTout)*(pTout))*(2.03914613-0.03906753*(pTout)+0.00045617*(pTout)*(pTout)-0.00000203*(pTout)*(pTout)*(pTout)))
															/ calculatedHeatingCapacity * KWPBTUPH;
						//itList->second.calculatedHeatingCOP = calculateHeatingCOP(itList->first);
						c_cop = *initCoolingCOP;

					}
					//if (calculatedLoad >= itList->second.heatingDemand){
						//itList->second.willWork = true;
					//}
							//printf("original load was : %f calculated load is : %f \n", itList->second.heatingDemand, calculatedLoad);
				}

				if (reCalculated && c_cop!=0 &&h_cop!=0){//
					printf("---------------------->>\r\n");
					// We have to cool
					if (*pMonitor > cool_max){
						double coolingCOP = c_cop;
						char coolingCOP_string[1024];
						sprintf(coolingCOP_string, "%f", coolingCOP);
						gl_set_value_by_name(parent2, "cooling_COP", coolingCOP_string);
						printf(" We have to cool :%f\r\n", c_cop);
					}
					// We have to heat
					else if (*pMonitor < heat_min){
						double heatingCOP = h_cop;
						char heatingCOP_string[1024];
						sprintf(heatingCOP_string, "%f", heatingCOP);
						gl_set_value_by_name(parent2, "heating_COP", heatingCOP_string);
						printf(" We have to heat : %f\r\n", h_cop);
					}
					// We might heat, if the price is right
					else if (*pMonitor <= heat_max && *pMonitor >= heat_min){
						double heatingCOP = h_cop;
						char heatingCOP_string[1024];
						sprintf(heatingCOP_string, "%f", heatingCOP);
						gl_set_value_by_name(parent2, "heating_COP", heatingCOP_string);
						printf(" We might have to heat :%f \r\n", h_cop);
					}
					// We might cool, if the price is right
					else if (*pMonitor <= cool_max && *pMonitor >= cool_min){
						double coolingCOP = c_cop;
						char coolingCOP_string[1024];
						sprintf(coolingCOP_string, "%f", coolingCOP);
						printf(" We might have to cool : %f \r\n", c_cop);
						gl_set_value_by_name(parent2, "cooling_COP", coolingCOP_string);
					}
					printf(" ------------------------<<\r\n");
				}
			}



*/




		////////////////////////////////////////////////////////////////////////////////

		// submit bids
		double previous_q = last_q; //store the last value, in case we need it
		last_p = 0.0;
		last_q = 0.0;
		
		// We have to cool
		if(*pMonitor > cool_max && (pThermostatState == NULL || *pThermostatState == 1 || *pThermostatState == 3)){
			last_p = *pPriceCap;
			last_q = *pCoolingDemand;
		}
		// We have to heat
		else if(*pMonitor < heat_min && (pThermostatState == NULL || *pThermostatState == 1 || *pThermostatState == 2)){
			last_p = *pPriceCap;
			last_q = *pHeatingDemand;
		}
		// We're floating in between heating and cooling
		else if(*pMonitor > heat_max && *pMonitor < cool_min){
			last_p = 0.0;
			last_q = 0.0;
		}
		// We might heat, if the price is right
		else if(*pMonitor <= heat_max && *pMonitor >= heat_min && (pThermostatState == NULL || *pThermostatState == 1 || *pThermostatState == 2)){
			double ramp, range;
			ramp = (*pMonitor > heating_setpoint0 ? heat_ramp_high : heat_ramp_low);
			range = (*pMonitor > heating_setpoint0 ? heat_range_high : heat_range_low);
			if(*pMonitor != heating_setpoint0){
				//printf("---6----->>>>:%f\r\n", pAvg);
				//printf("===>%f\r\n", averageClearingPrice);
				last_p = *pAvg + ( (fabs(*pStd) < bid_offset) ? 0.0 : (*pMonitor - heating_setpoint0) * ramp * (*pStd) / fabs(range) );
			} else {
				//printf("---7----->>>>:%f\r\n", pAvg);
				last_p = *pAvg;
			}
			last_q = *pHeatingDemand;
		}
		// We might cool, if the price is right
		else if(*pMonitor <= cool_max && *pMonitor >= cool_min && (pThermostatState == NULL || *pThermostatState == 1 || *pThermostatState == 3)){
			double ramp, range;
			ramp = (*pMonitor > cooling_setpoint0 ? cool_ramp_high : cool_ramp_low);
			range = (*pMonitor > cooling_setpoint0 ? cool_range_high : cool_range_low);
			if(*pMonitor != cooling_setpoint0){
				//printf("---8----->>>>:%f\r\n", pAvg);
				last_p = *pAvg + ( (fabs(*pStd) < bid_offset) ? 0 : (*pMonitor - cooling_setpoint0) * ramp * (*pStd) / fabs(range) );
			} else {
				//printf("---9----->>>>:%f\r\n", pAvg);
				last_p = *pAvg;
			}
			last_q = *pCoolingDemand;
		}

		if(last_p > *pPriceCap)
			last_p = *pPriceCap;
		if(last_p < -*pPriceCap)
			last_p = -*pPriceCap;
		if(0 != strcmp(market_unit, "")){
			if(0 == gl_convert("kW", market_unit, &(last_q))){
				gl_error("unable to convert bid units from 'kW' to '%s'", market_unit);
				return TS_INVALID;
			}
		}
		controller_bid.market_id = lastmkt_id;
		controller_bid.price = last_p;
		controller_bid.quantity = -last_q;


		///////////////////////////////////changing comfort zones of hvac//////////////////////////
		/*if (enthousiasm > 0.18){
					last_p = 0.0;
					last_q = 0.0;
				}*/




		if(last_q > 0.001){
			if( powerstate_prop.is_valid() ){
				if ( ps == *PS_ON ) {
					controller_bid.state = BS_ON;
				} else {
					controller_bid.state = BS_OFF;
				}
				((void (*)(char *, char *, char *, char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), (char *)(&pMkt), "submit_bid_state", "auction", (void *)&controller_bid, (size_t)sizeof(controller_bid));
				controller_bid.rebid = true;
			} else {
				controller_bid.state = BS_UNKNOWN;
				((void (*)(char *, char *, char *, char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), (char *)(&pMkt), "submit_bid_state", "auction", (void *)&controller_bid, (size_t)sizeof(controller_bid));
				controller_bid.rebid = true;
			}
			if(controller_bid.bid_accepted == false){
				return TS_INVALID;
			}
		}
		else
		{
			if (last_pState != ps)
			{
				KEY bid = (KEY)(lastmkt_id == *pMarketId ? lastbid_id : -1);
				double my_bid = -*pPriceCap;
				if ( ps != *PS_OFF  )
					my_bid = last_p;

				if ( ps == *PS_ON ) {
					controller_bid.state = BS_ON;
				} else {
					controller_bid.state = BS_OFF;
				}
				((void (*)(char *, char *, char *, char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), (char *)(&pMkt), "submit_bid_state", "auction", (void *)&controller_bid, (size_t)sizeof(controller_bid));
				if(controller_bid.bid_accepted == false){
					return TS_INVALID;
				}
				controller_bid.rebid = true;
			}
		}
	}

	if ( powerstate_prop.is_valid() )
		last_pState = ps;

	char timebuf[128];
	gl_printtime(t1,timebuf,127);
	//gl_verbose("controller:%i::sync(): bid $%f for %f kW at %s",hdr->id,last_p,last_q,timebuf);
	//return postsync(t0, t1);

	if (control_mode == CN_DEV_LEVEL) {		
		return fast_reg_run;
	}

	return TS_NEVER;
}

TIMESTAMP controller::postsync(TIMESTAMP t0, TIMESTAMP t1){
	TIMESTAMP rv = next_run - bid_delay;
	if(last_setpoint != setpoint0 && control_mode == CN_RAMP){
		last_setpoint = setpoint0;
	}
	if(last_setpoint != setpoint0 && control_mode == CN_DEV_LEVEL){
		last_setpoint = setpoint0;
	}
	if(last_heating_setpoint != heating_setpoint0 && control_mode == CN_DOUBLE_RAMP){
		last_heating_setpoint = heating_setpoint0;
	}
	if(last_cooling_setpoint != cooling_setpoint0 && control_mode == CN_DOUBLE_RAMP){
		last_cooling_setpoint = cooling_setpoint0;
	}

	// Determine the system_mode the HVAC is in
	if(t1 < next_run-bid_delay){
		return next_run-bid_delay;
	}

	if(resolve_mode == RM_SLIDING){
		if(*pHeatState == 1 || *pAuxState == 1){
			thermostat_mode = TM_HEAT;
			if(last_mode == TM_OFF)
				time_off = TS_NEVER;
		} else if(*pCoolState == 1){
			thermostat_mode = TM_COOL;
			if(last_mode == TM_OFF)
				time_off = TS_NEVER;
		} else if(*pHeatState == 0 && *pAuxState == 0 && *pCoolState == 0){
			thermostat_mode = TM_OFF;
			if(previous_mode != TM_OFF)
				time_off = t1 + dtime_delay;
		} else {
			gl_error("The HVAC is in two or more modes at once. This is impossible");
			if(resolve_mode == RM_SLIDING)
				return TS_INVALID; // If the HVAC is in two modes at once then the sliding resolve mode will have conflicting state input so stop the simulation.
		}
	}

	if (t1 - next_run < bid_delay){
		rv = next_run;
	}

	if(next_run == t1){
		next_run += (TIMESTAMP)(this->period);
		rv = next_run - bid_delay;
	}

	////////////////////////////////////////////////////////////////one house only///////////
	/*if (enthousiasm > 0.18){

					*pHeatingSetpoint = 0;
					*pCoolingSetpoint = 100;
	}*/


	return rv;
}

int controller::dev_level_ctrl(TIMESTAMP t0, TIMESTAMP t1){
	if (engaged == 1)
		if (last_market == 1)
			is_engaged = 1;
		else
			is_engaged = -1;
	else
		is_engaged = 0;
	
	OBJECT *hdr = OBJECTHDR(this);
	double my_id = hdr->id;

	// Not sure if this is needed, but lets clean up the Override signal if we are entering a new market
	//  We'll catch the new signal one reg signal too late for now
	if(((t1-last_run) % int(dPeriod)) == 0) {
		override_prop.setp(OV_NORMAL->get_enumeration_value());
		last_override = 0;
	}
	// If engaged and during the first pass, check to see if we should override
	else if (engaged==1)	{ 
		if ( t0 < t1 ) {
			// Only re-calculate this stuff on the defined regulation time
			if(((t1-last_run) % reg_period) == 0){

				// First time through this market period, grab some of the initial data
				if (t1 <=last_run + reg_period) {
					P_ON_init = P_ON;
					delta_u = fast_reg_signal*P_ON_init;		
					locked = 0;
					override_prop.setp(OV_NORMAL->get_enumeration_value());
					last_override = 0;
					u_last = (1+fast_reg_signal)*P_ON_init;
				}
				else {
					delta_u = (1+fast_reg_signal)*P_ON_init - u_last;
					u_last = (1+fast_reg_signal)*P_ON_init;
				}

				// if we are part of the OFF->ON market
				if (delta_u > 0 && last_market == 0) {
					if (locked == 0) {
						if (P_OFF != 0)
							mu0 = delta_u/P_OFF;
						else
							mu0 = 0;
						mu1 = 0;

						r1 = gl_random_uniform(RNGSTATE,0, 1.0);

						if(r1 < mu0){
							override_prop.setp(OV_ON->get_enumeration_value());
							locked = 1;
						} 
						else {
							if (use_override == OU_ON)
								override_prop.setp(OV_OFF->get_enumeration_value()); //keep it in the OFF position
							else
								override_prop.setp(OV_NORMAL->get_enumeration_value());	 //else operate normally is probably not needed
						}
					}
					else if (use_override == OU_ON) {
						override_prop.setp(OV_ON->get_enumeration_value()); //keep it in the ON position
					}
				}
				// Ensure that it stays in the position we have decided on
				else if (last_market == 0) {
					mu0=0;
					if (P_ON != 0.0)
						mu1=-delta_u/P_ON;
					else
						mu1 = 0;
					if (use_override == OU_ON) {
						if (locked == 1)
							override_prop.setp(OV_ON->get_enumeration_value()); //keep it in the ON position
						else
							override_prop.setp(OV_OFF->get_enumeration_value()); //keep it in the OFF position
					}
				}
				// If we are part of the ON->OFF market
				else if (delta_u < 0 && last_market == 1) {
					if (locked == 0) {
						mu0=0;
						if (P_ON != 0.0)
							mu1=-delta_u/P_ON;
						else
							mu1 = 0;

						r1 = gl_random_uniform(RNGSTATE,0, 1.0);

						if(r1 < mu1){
							override_prop.setp(OV_OFF->get_enumeration_value());
							locked = 1;
						} else {
							if (use_override == OU_ON)
								override_prop.setp(OV_ON->get_enumeration_value()); // keep it in the ON position
							else
								override_prop.setp(OV_NORMAL->get_enumeration_value());	// operate normally is probably not needed
						}	
					}
					else if (use_override == OU_ON) {
						override_prop.setp(OV_OFF->get_enumeration_value()); //keep it in the OFF position
					}
				}
				else if (last_market == 1) {
					if (P_OFF != 0)
							mu0 = delta_u/P_OFF;
						else
							mu0 = 0;
						mu1 = 0;
					if (use_override == OU_ON) {
						if (locked == 1)
							override_prop.setp(OV_OFF->get_enumeration_value()); //keep it in the OFF position
						else
							override_prop.setp(OV_ON->get_enumeration_value()); //keep it in the ON position
					}
				}
				else {
					mu0 = 0;
					mu1 = 0;
					override_prop.setp(OV_NORMAL->get_enumeration_value());
				}

				last_override = override_prop.get_integer();

				P_OFFLOCK = P_OFFLOCK + mu1*P_ON;			
				P_ON = (1-mu1)*P_ON;
				P_ONLOCK = P_ONLOCK + mu0*P_OFF;
				P_OFF = (1-mu0)*P_OFF; 
			}
			else {
				override_prop.setp(last_override);
			}
		}
		else {
			override_prop.setp(last_override);
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_controller(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(controller::oclass);
		if (*obj!=NULL)
		{
			controller *my = OBJECTDATA(*obj,controller);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(controller);
}

EXPORT int init_controller(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
		{
			return OBJECTDATA(obj,controller)->init(parent);
		}
		else
			return 0;
	}
	INIT_CATCHALL(controller);
}

EXPORT int isa_controller(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,controller)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_controller(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	controller *my = OBJECTDATA(obj,controller);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			//obj->clock = t1;
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock, t1);
			//obj->clock = t1;
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			obj->clock = t1;
			break;
		default:
			gl_error("invalid pass request (%d)", pass);
			return TS_INVALID;
			break;
		}
		return t2;
	}
	SYNC_CATCHALL(controller);
}




// EOF
