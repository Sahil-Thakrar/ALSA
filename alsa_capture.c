#include <alsa/asoundlib.h>
#include <stdio.h>
#include <signal.h>

snd_pcm_t *pcm_handle;
snd_pcm_hw_params_t *params;

FILE *fd;
int err;
unsigned int period_time=4000;//microseconds(4ms)
int periods=2;
int get_rate;
int dir;
int get_channels;
int write_data,read_data;
snd_pcm_state_t state;
char *position;

void set_hw_params(snd_pcm_hw_params_t *,snd_pcm_uframes_t,unsigned int,unsigned int,unsigned int);
void process_data(FILE *,void *,size_t,snd_pcm_uframes_t);
void status_check(snd_pcm_t *,snd_pcm_state_t,char *);
void state_number_to_name();
void sigint(int sig);           //controlling the ctrl-c interrupt signal

int main(int argc,char **argv)
{	
	if(argc!=5)
	{
		printf("Need arguments as \n1:Device Name \n2:Rate \n3:Channels \n4:input_file\n\n(as the command line arguments)\n");
		exit(EXIT_FAILURE);
	}

	signal(SIGINT, sigint);	
	unsigned int rate = atoi(argv[2]);
	snd_pcm_uframes_t frames=(rate*period_time)/1000000;
	char *device_name=argv[1];
	unsigned int channels = atoi(argv[3]);
	unsigned int buff_size=frames*channels*2;//sample size(16 bits(2bytes))
	int buff[(buff_size)/2];//for double byte size buffer
	
	fd=fopen(argv[4],"w");
	if(NULL==fd)
	{
		fprintf(stderr,"Can't open %s for writing\n",argv[4]);
		exit(EXIT_FAILURE);
	}

	err=snd_pcm_open(&pcm_handle,device_name,SND_PCM_STREAM_CAPTURE,0);
	if(err<0)
	{
		printf("ERROR: Can't open \"%s\" PCM device. %s\n",device_name,snd_strerror(err));
		exit(EXIT_FAILURE);
	}	

	while(1)
	{
		
		switch(snd_pcm_state(pcm_handle))
		{
			case SND_PCM_STATE_OPEN:
				//State is OPEN
				//state_number_to_name();
			case SND_PCM_STATE_SETUP:
				//State = SETUP
				set_hw_params(params,frames,rate,channels,buff_size);
				break;
			case SND_PCM_STATE_PREPARED:
				//State is PREPARED
				position="before start";
				//status_check(pcm_handle,state,position);	
				err=snd_pcm_start(pcm_handle);
				if(err<0)
				{
					printf("ERROR: Can't start pcm. %s\n", snd_strerror(err));
					exit(EXIT_FAILURE);
				}
				position= "after start";
				//status_check(pcm_handle,state,position);
				break;
			case SND_PCM_STATE_RUNNING:
				//State is RUNNING
				process_data(fd,buff,buff_size,frames);
				break;
			default:
				printf("What has to be done when this STATE (%s) arrives is not know \n",snd_pcm_state_name(state));
		}
	}		
}

void set_hw_params(snd_pcm_hw_params_t *params,snd_pcm_uframes_t frames,unsigned int rate,unsigned int channels,unsigned int buff_size)
{
	err=snd_pcm_hw_params_malloc(&params);
	if(err<0)
	{
		fprintf(stderr,"cannot allocate hardware parameter structure (%s)\n",snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	snd_pcm_hw_params_any(pcm_handle, params);

	err=snd_pcm_hw_params_set_access(pcm_handle,params,SND_PCM_ACCESS_RW_INTERLEAVED);
	if(err< 0) 
	{
		printf("ERROR: Can't set interleaved mode. %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	
	err = snd_pcm_hw_params_set_format(pcm_handle, params,SND_PCM_FORMAT_S16_LE); 
	if (err< 0) 
	{
		printf("ERROR: Can't set format. %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	
	err = snd_pcm_hw_params_set_channels(pcm_handle,params,channels);
	if (err< 0)
	{
		printf("ERROR: Can't set channels number. %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	
	err = snd_pcm_hw_params_set_rate(pcm_handle,params,rate,dir) ;
	if (err<0)
	{
		printf("ERROR: Can't set rate. %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	
	err=snd_pcm_hw_params_set_period_size(pcm_handle,params,frames,dir);
	if(err<0)
	{
		printf("ERROR: Can't set period size. %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	
	err=snd_pcm_hw_params_set_periods_near(pcm_handle,params,&periods,&dir);
	if(err<0)
	{
		printf("ERROR: Can't set periods value. %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	
	err=snd_pcm_hw_params_set_period_time_near(pcm_handle,params,&period_time,&dir);
	if(err<0)
	{
		printf("ERROR: Can't set period time. %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	err = snd_pcm_hw_params(pcm_handle, params); 
	if (err< 0)
	{
		printf("ERROR: Can't set harware parameters. %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	printf("PCM name :- '%s'\n", snd_pcm_name(pcm_handle));
	
	snd_pcm_hw_params_get_channels(params, &get_channels);
	printf("channels :- %i\n",get_channels);
	
	snd_pcm_hw_params_get_rate(params, &get_rate,0);
	printf("rate :- %d bps\n",get_rate);	
	
	snd_pcm_hw_params_get_period_time(params,&period_time,&dir);
	printf("Period Time :- %d\n",period_time);
	
	snd_pcm_hw_params_get_period_size(params,&frames,&dir);
	printf("Period Size :- %ld\n",frames);
	
	snd_pcm_hw_params_get_periods(params,&periods,&dir);
	printf("Periods :- %d\n",periods);	

	printf("Frames :- %ld\n",frames);
	printf("Buffer size :- %d\n",buff_size);
}

void sigint(int sig)           //controlling the ctrl-c interrupt signal
{
	err=snd_pcm_close(pcm_handle);
	if(err<0)
	{
		fprintf(stderr,"Unable to close the device");
		exit(EXIT_FAILURE);
	}
	printf("\n\nFound the Ctrl + C Signal\nSo Terminating the program\n\n");
	exit(EXIT_SUCCESS);	
}

void process_data(FILE *fd,void *buff,size_t buff_size,snd_pcm_uframes_t frames)
{
	position="before readi";
	//status_check(pcm_handle,state,position);
	read_data=snd_pcm_readi(pcm_handle,buff,frames);
	if (read_data == -EPIPE)
	{
		printf("Buffer is of small size\n");
		exit(EXIT_FAILURE);			
	}
	if(read_data<0)
	{
		fprintf(stderr,"Reading from the buffer is failed");
		exit(EXIT_FAILURE);
	}
	position="after readi";
	//status_check(pcm_handle,state,position);
	write_data=fwrite(buff,sizeof(int),read_data,fd);
	if(write_data<0)
	{
		fprintf(stderr,"Writing to the buffer is failed\n");
		exit(EXIT_FAILURE);
	}
	position="after writei";
	//status_check(pcm_handle,state,position);
}

void status_check(snd_pcm_t *pcm_handle,snd_pcm_state_t state,char *place)
{
	state = snd_pcm_state(pcm_handle);
	printf("State %s %s\n",place,snd_pcm_state_name(state));
}

void state_number_to_name()
{

	printf("State 0 %s \n",snd_pcm_state_name(0));
	printf("State 1 %s \n",snd_pcm_state_name(1));
	printf("State 2 %s \n",snd_pcm_state_name(2));
	printf("State 3 %s \n",snd_pcm_state_name(3));
	printf("State 4 %s \n",snd_pcm_state_name(4));
	printf("State 5 %s \n",snd_pcm_state_name(5));
	printf("State 6 %s \n",snd_pcm_state_name(6));
	printf("State 7 %s \n",snd_pcm_state_name(7));
	printf("State 8 %s \n\n",snd_pcm_state_name(8));
}