#include <alsa/asoundlib.h>
#include <stdio.h>
#include <signal.h>

snd_pcm_t *pcm_handle;
snd_pcm_hw_params_t *params;
int err;
unsigned int period_time=4000;//microseconds(4ms)
int periods=2;
int get_rate;
int dir;
int get_channels;
int read_data;
int fd;
snd_pcm_state_t state;
char *position;

void set_hw_params(snd_pcm_hw_params_t *,snd_pcm_uframes_t,unsigned int,unsigned int,unsigned int);
void process_data(int,int [],unsigned int,snd_pcm_uframes_t);
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

	unsigned int rate = atoi(argv[2]);
	snd_pcm_uframes_t frames=(rate*period_time)/1000000;
	char *device_name=argv[1];
	unsigned int channels = atoi(argv[3]);
	unsigned int buff_size=frames*channels*2;//sample size(16 bits(2bytes))
	int buff[(2*buff_size)];
	
	fd=open(argv[4],O_RDONLY,666);
	if(fd<0)
	{
		fprintf(stderr,"Can't open %s for reading\n",argv[2]);
		exit(EXIT_FAILURE);
	}
	err=snd_pcm_open(&pcm_handle,device_name,SND_PCM_STREAM_PLAYBACK,0);
	if(err<0)
	{
		printf("ERROR: Can't open \"%s\" PCM device. %s\n",device_name,snd_strerror(err));
		exit(EXIT_FAILURE);
	}	

	while(1)
	{
		signal(SIGINT, sigint);
		state=snd_pcm_state(pcm_handle);
		switch(state)
		{
			case 0:
				//state_number_to_name();
			case 1:
				set_hw_params(params,frames,rate,channels,buff_size);
				break;
			case 2:
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
			case 3:
				process_data(fd,buff,buff_size,frames);
				break;
			case 4:
				err = EPIPE;
				snd_pcm_prepare(pcm_handle);
				printf("Buffer is having Zero data.\n");
				break;
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
	err=snd_pcm_drain(pcm_handle);
	if(err<0)
	{
		fprintf(stderr,"Unable to drain all the data from the device");	
		exit(EXIT_FAILURE);
	}
	err=snd_pcm_close(pcm_handle);
	if(err<0)
	{
		fprintf(stderr,"Unable to close the device");
		exit(EXIT_FAILURE);
	}
	printf("\n\nFound the Ctrl + C Signal\nSo Terminating the program\n\n");
	exit(EXIT_SUCCESS);	
}

void process_data(int fd,int buff[],unsigned int buff_size,snd_pcm_uframes_t frames)
{
	read_data = read(fd,buff,buff_size);
	if(err<0)
	{
		fprintf(stderr,"Reading from the buffer is failed");
		exit(EXIT_FAILURE);
	}
	position="before writei";
	//status_check(pcm_handle,state,position);
	err = snd_pcm_writei(pcm_handle,buff,frames);
	if (err == -EPIPE)
	{
		snd_pcm_prepare(pcm_handle);
		printf("Buffer is having Zero data.\n");
	}
	if (err < 0)
	{
		printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	position="after writei";
	//status_check(pcm_handle,state,position);

	if(0==read_data)
	{
		printf("Audio is played completely...!\n\n");
		err=snd_pcm_close(pcm_handle);
		if(err<0)
		{
			fprintf(stderr,"Unable to close the device");
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);
	}
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

//with If statements 

// state=snd_pcm_state(pcm_handle);
	// if(state==0||state==1)
	// {
 		
	// 	printf("OPEN\n");
	// 	set_hw_params(params,frames,rate,channels,buff_size);
	// }	

	// state=snd_pcm_state(pcm_handle);
	// if(state==2)
	// { 
	// 	position="before start";
	// 	//status_check(pcm_handle,state,position);	
	// 	err=snd_pcm_start(pcm_handle);
	// 	if(err<0)
	// 	{
	// 		printf("ERROR: Can't start pcm. %s\n", snd_strerror(err));
	// 		exit(EXIT_FAILURE);
	// 	}
	// 	position= "after start";
	// 	//status_check(pcm_handle,state,position);
	// }
	// if(state==3)
	// {
	// 	err = EPIPE;
	// 	snd_pcm_prepare(pcm_handle);
	// 	printf("Buffer is having Zero data.\n");
	// }