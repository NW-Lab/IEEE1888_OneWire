/*
 * Copyright (c) 2013 Hideya Ochiai, the University of Tokyo,  All rights reserved.
 * 
 * Permission of redistribution and use in source and binary forms, 
 * with or without modification, are granted, free of charge, to any person 
 * obtaining the copy of this software under the following conditions:
 * 
 *  1. Any copies of this source code must include the above copyright notice,
 *  this permission notice and the following statement without modification 
 *  except possible additions of other copyright notices. 
 * 
 *  2. Redistributions of the binary code must involve the copy of the above 
 *  copyright notice, this permission notice and the following statement 
 *  in documents and/or materials provided with the distribution.  
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*                                                                                */
/* ieee1888_onewire_app.c                                                          */
/*                                                                                */
/* author: Yasushi Tauchi                                                          */
/* create: 2015-05-21                                                             */
/* update: 2018-05-29                                                             */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include "ieee1888.h"


#define MAX_POINT_ID_LEN 1024
#define MAX_CONTENT_LEN 6
#define MAX_TIME_LEN 32
#define POINT_COUNT 200

struct point_value {
  char point_id[MAX_POINT_ID_LEN];
  time_t time;
  char value[MAX_CONTENT_LEN];
};
struct OneWireSensor{
  char OneWireID[18];
  char pointID[MAX_POINT_ID_LEN];
};
struct point_value m_buffer[POINT_COUNT];
struct OneWireSensor m_OneWireSensor[POINT_COUNT];

int No_OneWireSensor=0;
int No_ConnectSensor=0;
//char m_IEEE1888_POINTID[]="http://www.test.fiap.jp/raspberrypi/";
char m_IEEE1888_POINTID[100];
//char m_IEEE1888_TO[]="http://fiap-dev.gutp.ic.i.u-tokyo.ac.jp/axis2/services/FIAPStorage";
char m_IEEE1888_TO[100];
char m_OneWire_path[]="/sys/bus/w1/devices";

int init(){
	FILE *fp;
        int i, ret;
        char x1[100],x2[100],x3[100],x4[100],x5[100];
	printf("**fiap configfile read \n");
//fiap.config
	fp=fopen("/etc/FIAP/fiap.config","r");
        if(fp==NULL){
                printf("fiap.config File Open Error!\n");
                return -1;
        }
        while(fscanf(fp,"%[^;];%s",x1,x2)!=EOF){
	if(x1[0]==10)
		for (int k=1;k<strlen(x1)+1;k++)x1[k-1]=x1[k];
        if(x1[0]!='#'){
			if(strcmp(x1,"SERVER")==0)strcpy(m_IEEE1888_TO,x2);
			if(strcmp(x1,"POINTSET")==0)strcpy(m_IEEE1888_POINTID,x2);
                }
        }
        fclose(fp);
	printf(" SERVER:%s \n",m_IEEE1888_TO);
	printf(" POINTSET:%s \n",m_IEEE1888_POINTID);
        printf("********************************\n");

//DS18B20.DAT
	fp=fopen("/etc/FIAP/DS18B20.DAT","r");
	if(fp==NULL){
		printf("DS18B20.DAT File Open Error!\n");
		return -1;
	}
	while(fscanf(fp,"%[^,],%[^,],%[^,],%[^,],%s",x1,x2,x3,x4,x5)!=EOF){
		if((x1[0]!='#')&&(strlen(x2)!=0)){
			strcpy(m_OneWireSensor[No_OneWireSensor].OneWireID,x3);
			sprintf(m_OneWireSensor[No_OneWireSensor].pointID,"%s%s",m_IEEE1888_POINTID,x2);
			No_OneWireSensor++;
		}
		strcpy(x1,"");
		strcpy(x2,"");
		strcpy(x3,"");
		strcpy(x4,"");
		strcpy(x5,"");
	}
	fclose(fp);
// OneWire Device
	char dev[16],devPath[128],buf[256],tmpData[10];
	DIR *dir;
	struct dirent *dirent;
	ssize_t numRead;
	float tempC;
	int fd;
	time_t t=time(NULL);
	dir=opendir(m_OneWire_path);
	if (dir!=NULL){
		while((dirent=readdir(dir))){
			if(dirent->d_type==DT_LNK && strstr(dirent->d_name,"28-")!=NULL){
				strcpy(dev,dirent->d_name);
				for(i=0;i<No_OneWireSensor;i++){
					if(strcasecmp(dev,m_OneWireSensor[i].OneWireID)==0)
						break;
				}
				if(i==No_OneWireSensor){
					printf("Device Not Reg:%s\n",dev);
				}else{
					sprintf(devPath,"%s/%s/w1_slave",m_OneWire_path,dev);
					fd=open(devPath,O_RDONLY);
					if(fd==-1){
						printf("file not found %s \n",devPath);
					}else{
						while((numRead=read(fd,buf,256))>0){
							strncpy(tmpData,strstr(buf,"t=")+2,5);
							tempC=strtof(tmpData,NULL);
							tempC=tempC/1000;
							
						}
						close(fd);
						//IEEE1888S DATA
						strcpy(m_buffer[No_ConnectSensor].point_id,m_OneWireSensor[i].pointID);
						m_buffer[No_ConnectSensor].time=t;
						sprintf(m_buffer[No_ConnectSensor].value,"%4.1f",tempC);
						No_ConnectSensor++;
						printf("Device:%s PointID:%s Temp:%4.1f \n",dev,m_OneWireSensor[i].pointID,tempC);
					}
				}
			}
		}
		(void)closedir(dir);
	}
	else{
		printf("W1device Directory Nothing\n");
		return -1;
	}
}

#define IEEE1888_WRITE_SUCCESS 1
#define IEEE1888_WRITE_FAIL 0
int write_to_server(){

   ieee1888_transport* rq_transport=ieee1888_mk_transport();
   ieee1888_body* rq_body=ieee1888_mk_body();
   ieee1888_point* rq_point=ieee1888_mk_point_array(No_ConnectSensor);
   
   int i;
   for(i=0;i<No_ConnectSensor;i++){
       ieee1888_value* rq_value=ieee1888_mk_value();
       rq_value->time=ieee1888_mk_time(m_buffer[i].time);
       rq_value->content=ieee1888_mk_string(m_buffer[i].value);
       rq_point[i].id=ieee1888_mk_uri(m_buffer[i].point_id);
       rq_point[i].value=rq_value;
       rq_point[i].n_value=1;
	}
   
   rq_body->point=rq_point;
   rq_body->n_point=No_ConnectSensor;
   rq_transport->body=rq_body;

   // for Debug
   // ieee1888_dump_objects((ieee1888_object*)rq_transport);

   int err;
   ieee1888_transport* rs_transport=ieee1888_client_data(rq_transport,m_IEEE1888_TO,0,&err);

   // Recycle
   ieee1888_destroy_objects((ieee1888_object*)rq_transport);
   free(rq_transport);

   if(rs_transport==NULL){
     return IEEE1888_WRITE_FAIL;
	}

	ieee1888_header* rs_header=rs_transport->header;
	if(rs_header==NULL){
		ieee1888_destroy_objects((ieee1888_object*)rs_transport);
		free(rs_transport);
		return IEEE1888_WRITE_FAIL;
	}

	if(rs_header->OK==NULL){
		if(rs_header->error==NULL){
			printf("FATAL: neither OK nor error was put in the response.\n");
		}
		ieee1888_destroy_objects((ieee1888_object*)rs_transport);
		free(rs_transport);
		return IEEE1888_WRITE_FAIL;
	}
  
	ieee1888_destroy_objects((ieee1888_object*)rs_transport);
	free(rs_transport);

	return IEEE1888_WRITE_SUCCESS;
}

int main(int argc,char** argv){
	printf("**********IEEE1888 OneWire Program start********** \n");
	if(init()==-1){
		printf("Error \n");
		sleep(15);
		return -1;
	}
   if(write_to_server()==IEEE1888_WRITE_SUCCESS){
		printf("FIAP Write -- Success!\n");
		sleep(1);
		return 0;
	}else{
		printf("FIAP Write -- ERROR!\n");
		sleep(15);
		return -1;
	}
}


