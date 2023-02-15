#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include "wifi_tools.h"

struct wifi_struct
{
	char cellNo[12];
    char bssid[48];
	char essid[64];
	char key[100];
	char quality[100];
	char signalLevel[100];
	char wpa1[200];
    char wpa2[300];
};

int PatternMatch(char *bematch, char *pattern, char *match)
{
	char errbuf[1024];                                                                    
	regex_t reg;                                       
	int err,nm = 10;                                   
	regmatch_t pmatch[nm];                             
		                                               
	if(regcomp(&reg,pattern,REG_EXTENDED) < 0)
	{        
		regerror(err,&reg,errbuf,sizeof(errbuf));          
		printf("err:%s\n",errbuf); 
		return -1;                        
	}                                                  
		                                               
	err = regexec(&reg,bematch,nm,pmatch,0);           
		                                               
	if(err == REG_NOMATCH){                            
		printf("no match\n");                              
		return -1;                                            
	}else if(err){                                     
		regerror(err,&reg,errbuf,sizeof(errbuf));          
		printf("err:%s\n",errbuf);                         
		return -1;                                          
	}                                                  
	
	int i;                                               
	for(i=0;i<nm && pmatch[i].rm_so!=-1;i++)
	{      
		int len = pmatch[i].rm_eo-pmatch[i].rm_so;         
		if(len)
		{                                           
			memcpy(match,bematch+pmatch[i].rm_so,len);                                   
		}                                                  
	} 
	return 1;        
}

void SegregateMsg(char * msg, struct wifi_struct *ptswifi, int index)
{
	char *p, *p2, *ptr;
	char *pattern;
	char match[100];
	
	memset(match, '\0', sizeof(match));
	
	//SSID
	ptr = strstr(msg, "ESSID");
	if(ptr)
	{
		p = strtok(msg, "\"");
		p = strtok(NULL, "\"");
		if(p){		
			strcpy(ptswifi[index].essid, p);
		}
	}
	
	//wap1
	ptr = strstr(msg, "WPA Version");
	if(ptr)
	{ 
		p = strtok(msg," ");
		p = strtok(NULL, "\n");
		if(p){				
			strcpy(ptswifi[index].wpa1, p);
		}  
	}
 
	//wpa2
	ptr = strstr(msg, "WPA2 Version");
	if(ptr)
	{ 
		p = strtok(msg," ");
		p = strtok(NULL, "\n");
		if(p){				
			strcpy(ptswifi[index].wpa2, p);
		}  
	}
	
	//key:PSK
	ptr = strstr(msg, "Authentication Suites");
	if(ptr)
	{
		pattern = "^Authentication.{14}(.*)\n$";
		memset(match,0,sizeof(match));
		PatternMatch(ptr, pattern, match); 
		strcpy(ptswifi[index].key, match);
	}
	
	//quality  ||   signal level
	ptr = strstr(msg, "Quality");
	if(ptr)
	{
		p = strtok(msg, " ");
		p2 = strtok(NULL, " ");
		p2 = strtok(NULL, " ");
		
		//quality
		p = strtok(p, "=");
		p = strtok(NULL, "=");
		if(p){		
			strcpy(ptswifi[index].quality, p);
		}

		//signal level
		p = strtok(p2, "=");
		p = strtok(NULL, "=");
		if(p){		
			strcpy(ptswifi[index].signalLevel, p);
		}
	}
}

//scanning
int ScanApp(char * cmd, struct wifi_struct *ptswifi) 
{ 
	FILE * fp; 
	int res;
	char *p, *ptr, *a;
	int index;
	char buf[1024]; 
	
	if (cmd == NULL) 
	{ 
		printf("cmd is NULL!\n");
		return -1;
	} 
	if ((fp = popen(cmd, "r") ) == NULL) 
	{ 
		perror("popen");
		printf("popen error: %s/n", strerror(errno)); 
		return -1; 
	} 
	else
	{
		while(fgets(buf, sizeof(buf), fp)) 
		{ 
			//cell no
			ptr = strstr(buf, "Cell");
			if(ptr)
			{
				p = strtok(buf, " ");
				p = strtok(NULL, " ");
				if(p){
					index = atoi(p);
					strcpy(ptswifi[index].cellNo, p);			
				}
				a= strtok(NULL, "Address:");
				a= strtok(NULL, " ");
				a = strtok(NULL, "\n");
				if(a){		
					 strcpy(ptswifi[index].bssid,a);
				}
			}

			SegregateMsg(buf, ptswifi, index);  //segregate the message
		} 
		if ( (res = pclose(fp)) == -1) 
		{ 
			printf("close popen file pointer fp error!\n"); 
			return res;
		} 
		else if (res == 0) 
		{
			return res;
		} 
		else 
		{ 
			printf("popen res is :%d\n", res); 
			return res; 
		} 
	}
}
int GetWifiQuality(char *pWifiName,int *piQuality)
{
	int i;
	struct wifi_struct tswifi[100];
	int quality0 = 0,qualityFull = 70;
	ScanApp("iwlist wlan0 scanning | grep -E 'Cell|ESSID|PSK|Version|Quality'", tswifi);

	i = 1;
	while(strcmp(tswifi[i].cellNo,"")!=0)
	{
		if(strcmp(pWifiName,tswifi[i].essid) == 0)
		{
			sscanf(tswifi[i].quality,"%d/%d",&quality0,&qualityFull);
			*piQuality = (quality0 * 100)/qualityFull;
			//printf("tswifi[i].quality:%s;quality0:%d,qualityFull:%d\n",tswifi[i].quality,quality0,qualityFull);
			//printf("%s,%s,%s,%s,%s,%s,%s,%s\n",tswifi[i].cellNo,tswifi[i].essid,tswifi[i].key,tswifi[i].quality,tswifi[i].signalLevel,tswifi[i].wpa1,tswifi[i].wpa2,tswifi[i].bssid);
			return 0;
		}
		i++;
	}
	return -1;
}

