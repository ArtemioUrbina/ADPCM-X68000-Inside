#include <stdio.h>
#include <dos.h>
#include <stdlib.h>

/* from inside x68000 book page 298 trasncribed and adapted by Artemio Urbina*/

struct DMAREG {
	unsigned char csr;
	unsigned char cer;
	unsigned short sparel;
	unsigned char dcr;
	unsigned char ocr;
	unsigned char scr;
	unsigned char ccr;
	unsigned short spare2; 
	unsigned short mtc;
	unsigned char *mar;
	unsigned long spare3;
	unsigned char *dar;
	unsigned short spare4;
	unsigned short btc;
	unsigned char *bar;
	unsigned long spare5;
	unsigned char spare6; 
	unsigned char niv;
	unsigned char spare7;
	unsigned char eiv;
	unsigned char spare8;
	unsigned char mfc;
	unsigned short spare9; 
	unsigned char spare10;
	unsigned char cpr;
	unsigned short spare11; 
	unsigned char spare12;
	unsigned char dfc;
	unsigned long spare13;
	unsigned short spare14;
	unsigned char spare15;
	unsigned char bfc;
	unsigned long spare16;
	unsigned char sparel7;
	unsigned char gcr;
};

volatile struct DMAREG *dma;
volatile unsigned char *ppi_cwr;
volatile unsigned char *opm_regno;
volatile unsigned char *opm_data;
volatile unsigned char *adpcm_command;
volatile unsigned char *adpcm_status;
volatile unsigned char *adpcm_data ;

#define BUFSIZE 0x400
unsigned char pcmbuf [BUFSIZE];

void create_adpcmdata(unsigned char *buf, unsigned int length)
{
	while(length--)
		*buf++ = 0x1f;
}

void adpcm_outsel(unsigned int sel)
{
	*ppi_cwr = (0 << 1) | ((sel >> 1) & 1); /* Left */
	*ppi_cwr = (1 << 1) | (sel & 1); /* Right */
}

/* 10 -> 1/512  (7.8k/4Mhz 15.6k/8Mhz) */
/* 01 -> 1/768  (5.2k/4Mhz 10.4k/8Mhz) */
/* 00 -> 1/1024 (3.9k/4Mhz 7.8k/8Mhz) */
void adpcm_sample(unsigned int rate)
{
	*ppi_cwr = (2 << 1) | ((rate >> 1) & 1);
	*ppi_cwr = (3 << 1) | (rate & 1);
}

/* OPM 0x1B sets clk, 0 -> 8Mhz 1 -> 4Mhz) */
void adpcm_clksel(unsigned int sel)
{
	*opm_regno = 0x1b;
	*opm_data = (sel & 1) << 7;
}

void adpcm_stop()
{
	*adpcm_command = 0x1;
}

void adpcm_start()
{
	*adpcm_command = 0x2;
}

void dma_setup()
{
	dma->dcr = 0x80;
	dma->ocr = 0x32;
	dma->scr = 0x04;
	dma->ccr = 0x00;
	dma->cpr = 0x08;
	dma->mfc = 0x05;
	dma->dfc = 0x05;

	dma->mtc = BUFSIZE;
	dma->mar = pcmbuf;
	dma->dar = (unsigned char *)adpcm_data;
}

void dma_start()
{
	dma->ccr |= 0x80;
}

void wait_complete()
{
	while(!(dma->csr & 0x90) && ! (*adpcm_status & 0x80));
}

void clear_flag()
{
	dma->csr = 0xff;
}

int main(int argc, char *argv[])
{
	unsigned int i, pan, sample, clk;
	if (argc >= 2)
		pan = atoi(argv[1]);
	else pan = 0;
	if (argc >= 3)
		sample = atoi(argv[2]);
	else sample = 0;
	if (argc >= 4)
		clk = atoi(argv[3]);
	else clk = 0;

	_dos_super(0);
	dma            = (struct DMAREG *)0xe840c0;
	ppi_cwr        = (unsigned char *)0xe9a007;
	opm_regno      = (unsigned char *)0xe90001;
	opm_data       = (unsigned char *)0xe90003;
	adpcm_command  = (unsigned char *)0xe92001;
	adpcm_status   = (unsigned char *)0xe92001;
	adpcm_data     = (unsigned char *)0xe92003;

	adpcm_stop();
	create_adpcmdata(pcmbuf, BUFSIZE);
	adpcm_outsel(pan); /* Panpot Control */
	adpcm_sample(sample); /* Sampling rate */
	adpcm_clksel(clk); /* ADPCM Clock */
	clear_flag();

	dma_setup();
	dma_start();
	adpcm_start();
	wait_complete();
	adpcm_stop();
	clear_flag();
	return 0;
}

