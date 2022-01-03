#include <alsa/asoundlib.h>
#include <stdio.h>
#include <signal.h>

FILE *file =NULL;
snd_pcm_t *pcm_handle;
snd_pcm_hw_params_t *params;
int err;

void sigint(int sig)           //controlling the ctrl-c interrupt signal
{
	printf("\n\nFound the Ctrl + C Signal\nSo Terminating the program\n\n");
	err=snd_pcm_drain(pcm_handle);
	if(err<0)
	{
		fprintf(stderr,"Unable to drain all the data from the device");
	}
	err=snd_pcm_close(pcm_handle);
	if(err<0)
	{
		fprintf(stderr,"Unable to close the device");
	}
	exit(0);
	
}
int main(int argc,char **argv)
{	
	if(argc!=5)
	{
		printf("Need arguments as \n1:Device Name \n2:Rate \n3:Channels \n4:input_file\n(as the command line arguments) \n");
		exit(1);
	}

	unsigned int period_time=4000;//microseconds(4ms)
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames=192;
	char *device_name=argv[1];
	unsigned int rate = atoi(argv[2]);
	unsigned int channels = atoi(argv[3]);
	unsigned int buff_size=frames*channels*2;//sample size(16 bits(2bytes))
	int buff[buff_size];
	int periods=2;
	int get_rate;
	int dir;
	int get_channels;
	
	//file=fopen(argv[4],"r");
	int fd=open(argv[4],O_RDONLY,666);
	//if(NULL==file)
	//{
	//	fprintf(stderr,"Can't open %s for reading\n",argv[2]);
//		exit(1);
//	}

	err=snd_pcm_open(&pcm_handle,device_name,SND_PCM_STREAM_PLAYBACK,0);
	if(err<0)
	{
		printf("ERROR: Can't open \"%s\" PCM device. %s\n",device_name,snd_strerror(err));
	}
	
	err=snd_pcm_hw_params_malloc(&params);
	if(err<0)
	{
		fprintf(stderr,"cannot allocate hardware parameter structure (%s)\n",snd_strerror(err));
		exit(1);
	}
	snd_pcm_hw_params_any(pcm_handle, params);
	
	err=snd_pcm_hw_params_set_access(pcm_handle,params,SND_PCM_ACCESS_RW_INTERLEAVED);
	if(err< 0) 
	{
		printf("ERROR: Can't set interleaved mode. %s\n", snd_strerror(err));
	}
	
	err = snd_pcm_hw_params_set_format(pcm_handle, params,SND_PCM_FORMAT_S16_LE); 
	if (err< 0) 
	{
		printf("ERROR: Can't set format. %s\n", snd_strerror(err));
	}
	
	err = snd_pcm_hw_params_set_channels(pcm_handle,params,channels);
	if (err< 0)
	{
		printf("ERROR: Can't set channels number. %s\n", snd_strerror(err));
	}
	
	err = snd_pcm_hw_params_set_rate(pcm_handle,params,rate,dir) ;
	if (err<0)
	{
		printf("ERROR: Can't set rate. %s\n", snd_strerror(err));
	}
	
	err=snd_pcm_hw_params_set_period_size(pcm_handle,params,frames,dir);
	if(err<0)
	{
		printf("ERROR: Can't set period size. %s\n", snd_strerror(err));
	}
	
	err=snd_pcm_hw_params_set_periods_near(pcm_handle,params,&periods,&dir);
	if(err<0)
	{
		printf("ERROR: Can't set periods value. %s\n", snd_strerror(err));
	}
	
	err=snd_pcm_hw_params_set_period_time_near(pcm_handle,params,&period_time,&dir);
	if(err<0)
	{
		printf("ERROR: Can't set period time. %s\n", snd_strerror(err));
	}
	
	err = snd_pcm_hw_params(pcm_handle, params); 
	if (err< 0)
	{
		printf("ERROR: Can't set harware parameters. %s\n", snd_strerror(err));
	}

	printf("PCM name: '%s'\n", snd_pcm_name(pcm_handle));
	snd_pcm_hw_params_get_channels(params, &get_channels);
	printf("channels: %i",get_channels);
	if (1== get_channels)
	{
		printf("(mono)\n");
	}
	else if (2==get_channels)
	{
		printf("(stereo)\n");
	}
	
	snd_pcm_hw_params_get_rate(params, &get_rate,0);
	printf("rate: %d bps\n",get_rate);	
	
	snd_pcm_hw_params_get_period_time(params,&period_time,&dir);
	printf("Period Time :- %d\n",period_time);
	
	snd_pcm_hw_params_get_period_size(params,&frames,&dir);
	printf("Period Size:- %ld\n",frames);
	
	snd_pcm_hw_params_get_periods(params,&periods,&dir);
	printf("Periods %d\n",periods);	
	
	printf("Buffer size:- %d\n",buff_size);
	
	while(1)
	{
		err = read(fd,buff,buff_size);
		if(err<0)
		{
			fprintf(stderr,"Reading from the buffer is failed");
		}
		signal(SIGINT, sigint);
		err = snd_pcm_writei(pcm_handle,buff,frames);
		if (err == -EPIPE)
		{
			printf("Buffer is having Zero data.\n");
			snd_pcm_prepare(pcm_handle);
			
		}
		else if (err < 0)
		{
			printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(err));
		}

	}
}