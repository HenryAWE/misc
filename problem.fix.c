//	Use gdb to debug this program.
/*
	In this program, which converts a sample rate of 2n of a wav file to a
	sample rate of n, there are some bugs. You can use gnu gdb to debug.
	In this program, we use standard C functions when reading data from the
	origin file, and when we write data to the .wav file, we use POSIX
	fonctions. We also give a POSIX implementation as annotation when
	reading and a standard C implementation when writing. We hope that you
	can understand both implementation.
	A copy of a wav file format note is in 
		   https://hydrogen.dorado.cc/WavFormatDocs.pdf
	You can read it if you are interested.
	You can try this program using the wav file in the compressed file:
		   https://hydrogen.dorado.cc/audio1.tar.xz
		or https://lithium.dorado.cc/audio1.tar.xz (ipv6 only)
		(they are the same file saved in different servers)
	It is adviced that you download them using CAMPUS NETWORK(LZU or iLZU)
	while other network like ChinaNet-WiFi, CMCC-WiFi is also compatible.
*/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<inttypes.h>
#include<fcntl.h>  // a POSIX header file containing open()
#include<unistd.h> // a POSIX header file containing 
                       // write() read() lseek() close()

#pragma pack(1) //To sets the current packing alignment value to 1 byte.

typedef struct _RIFF_t{  //uint8_t means an unsigned 8bit data, which is
	uint8_t chunk_id[4];   //normally unsigned char in x86-64 linux gcc
	uint32_t chunksize;  //uint32_t means an unsigned 32bit data.
	uint8_t format[4];	 //these types are defined in inttypes.h
} RIFF_t;

typedef struct _FMT_t{
	uint8_t subchk_id[4];
	uint32_t subchunk_size;
	uint16_t format;
	uint16_t channels;
	uint32_t SampleRate;
	uint32_t ByteRate;
	uint16_t align;
	uint16_t bps;
} FMT_t;

typedef struct _DATA_t{
	uint8_t subchk_id[4];
	uint32_t subchunk_size;
	uint8_t * pcmdata;
} DATA_t;

typedef struct _WAV_t{
	RIFF_t riff;
	FMT_t fmt;
	DATA_t data;
} WAV_t;

int write_wav();
size_t readfile();
int process();

int main(int argc, char ** argv)
{
	WAV_t in, out;
	int infilesize, datasize;
	int fdwav;
	char datachunc[8];
	char * filebuffer;

	infilesize = readfile(&filebuffer, argv[1]);

	memcpy(&in, filebuffer, 44);
	in.data.pcmdata = (uint8_t*)malloc(infilesize-44);
	memcpy(in.data.pcmdata, filebuffer+44, infilesize-44);
	free(filebuffer);

	process(in, &out);
	//free(in.data.pcmdata);

	write_wav(argv[2], out, datasize);

	return 0;
}

size_t readfile(char ** buffer, const char * const filename)
{
	FILE * fp; //int fd;
	size_t lenth;

	fp = fopen(filename, "r"); //fd = open(filename, O_RDONLY);

	fseek(fp, 0, SEEK_END);
	lenth = ftell(fp); //lenth = lseek(fd, 0, SEEK_END); (see note 1)
	rewind(fp); //lseek(fd, 0, SEEK_SET);
		//you can also use function stat() to get the file size in POSIX

	char* content = malloc(lenth);
	content[lenth-1] = 0;

	fread(content, 1, lenth, fp); //read(fd, buffer, lenth);

	fclose(fp);//close(fd);
	*buffer = content;
	return lenth;
}//note: Upon successful completion, lseek() returns directly the resulting offset location as measured in bytes from the beginning of the file, while fseek() return 0. They both returns -1 when error occurs.

int write_wav(const char * const fname, WAV_t wavh)
{
	int fd; //FILE * fp;

	fd = open(fname, O_WRONLY | O_CREAT, 0644);//fp = fopen(fname, "r+");

	write(fd, &wavh, 44); //fwrite(&wavh,44,1,fp);
	write(fd, wavh.data.pcmdata, wavh.data.subchunk_size); //fwrite(data,size,1,fp);

	close(fd);	//fclose(fp);

	free(wavh.data.pcmdata);
	return 0;
}

int process(WAV_t i, WAV_t *o)
{
	WAV_t t = i;
	int datasize = i.data.subchunk_size;
	int framesize = i.fmt.align;
	int ctr;

	for(ctr = 0; (ctr*2) < datasize; ctr+=framesize)
	{
		memcpy((t.data.pcmdata)+ctr, i.data.pcmdata+2*ctr, framesize);
	}
	if(datasize/framesize)
		memcpy(t.data.pcmdata+ctr,i.data.pcmdata+2*ctr+framesize,
				framesize);

	t.fmt.SampleRate = i.fmt.SampleRate/2;
	t.fmt.ByteRate = t.fmt.SampleRate * t.fmt.channels * t.fmt.bps/8;
	t.data.subchunk_size = ctr + framesize;
	t.riff.chunksize= t.data.subchunk_size + 36;
	*o = t;
	return 0;
}
