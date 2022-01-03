#include <alsa/asoundlib.h>
#include <stdio.h>
#include <signal.h>

FILE *file =NULL;
snd_pcm_t *capture_handle;
int err;

void sigint(int sig)           //controlling the ctrl-c interrupt signal
{
	printf("\n\nFound the Ctrl + C Signal\nSo Terminating the program\n\n");
	err=snd_pcm_close(capture_handle);
	if(err<0)
	{
		fprintf(stderr,"Unable to close the handle");
	}
	err=fclose(file);
	if(err<0)
	{
		fprintf(stderr,"Unable to close the file");
	}
	exit(0);
}

int main(int argc,char **argv)
{
	if(argc!=5)
	{
		printf("Need arguments as \n1:Device Name \n2:Rate \n3:Channels \n4:output_file\n(as the command line arguments) \n");
		exit(1);
	}

	unsigned int period_time=4000;//microseconds(4ms)
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames=192;
	int i_loops;
	char *device_name=argv[1];
	unsigned int rate = atoi(argv[2]);
	unsigned int channels = atoi(argv[3]);
	unsigned int buff_size=frames*channels*2;//sample size(16 bits(2bytes))
	int buff[buff_size];
	int read_data,dir;
	int periods=2;

	file=fopen(argv[4],"w");
	if(NULL==file)
	{
		fprintf(stderr,"Can't open %s for writing\n",argv[1]);
		exit(1);
	}
	
	err=snd_pcm_open(&capture_handle,device_name,SND_PCM_STREAM_CAPTURE,0);
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
	
	err=snd_pcm_hw_params_any(capture_handle, params);
	if(err<0)
	{
		fprintf(stderr,"cannot allocate hardware parameter structure (%s)\n",snd_strerror(err));
		exit(1);
	}
	
	err=snd_pcm_hw_params_set_access(capture_handle,params,SND_PCM_ACCESS_RW_INTERLEAVED);
	if(err< 0) 
	{
		printf("ERROR: Can't set interleaved mode. %s\n", snd_strerror(err));
	}
	
	err = snd_pcm_hw_params_set_format(capture_handle, params,SND_PCM_FORMAT_S16_LE); 
	if (err< 0) 
	{
		printf("ERROR: Can't set format. %s\n", snd_strerror(err));
	}
	
	err = snd_pcm_hw_params_set_channels(capture_handle,params,channels); 
	if (err< 0)
	{
		printf("ERROR: Can't set channels number. %s\n", snd_strerror(err));
	}
	
	err = snd_pcm_hw_params_set_rate(capture_handle,params,rate,dir) ;
	if (err<0)
	{
		printf("ERROR: Can't set rate. %s\n", snd_strerror(err));
	}
	
	err=snd_pcm_hw_params_set_period_size(capture_handle,params,frames,dir);
	if(err<0)
	{
		printf("ERROR: Can't set period size. %s\n", snd_strerror(err));
	}
		
	err=snd_pcm_hw_params_set_periods_near(capture_handle,params,&periods,&dir);
	if(err<0)
	{
		printf("ERROR: Can't set periods value. %s\n", snd_strerror(err));
	}
	
	err=snd_pcm_hw_params_set_period_time_near(capture_handle,params,&period_time,&dir);
	if(err<0)
	{
		printf("ERROR: Can't set period time. %s\n", snd_strerror(err));
	}
	
	err = snd_pcm_hw_params(capture_handle, params); 
	if (err< 0)
	{
		printf("ERROR: Can't set harware parameters. %s\n", snd_strerror(err));
	}
	
	printf("Audio is being recorded from %d Channels\n",channels);
	printf("Audio is being recorded at %d Period Time \n",period_time);
	printf("Audio is being recorded at %ld Period size\n",frames);
	printf("Audio is being recorded at %d Periods\n",periods);
	printf("Audio is being recorded in %d size Buffer \n",buff_size);
	
	while(1)
	{
		read_data=snd_pcm_readi(capture_handle,buff,buff_size);
		if (read_data== -EPIPE)
		{
			printf("Buffer is of small size\n");			
		}
		else if(read_data<0)
		{
			fprintf (stderr,"read from audio interface failed (%s)\n",snd_strerror(err));
			snd_pcm_prepare(capture_handle);
		}
		else
		{
			fwrite(buff,sizeof(int),read_data,file);
		}
	}
	return 0;
}
