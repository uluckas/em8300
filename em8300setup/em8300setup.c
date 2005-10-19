/*     em8300setup 
 *     For microcode uploading to, and configuration of, your em8300 based hardware mpeg2 decoder card.
 *     Copyright (C) 2002 Malcolm Lashley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

extern int errno;

#include <linux/em8300.h>

int check_errno(char * s) {
	if(errno == ENOTTY) 
		printf("%s : em8300 Driver returned ENOTTY from ioctl - perhaps you need to upload the microcode 1st?\n",s);
	else
		printf("%s : em8300 Driver returned %i (unknown)\n",s,errno);

	return 0;
}

int display_current_settings (int DEV) {
        int tvmode=-1, aspect=-1, audio=-1, spu=-1;
	if((ioctl(DEV, EM8300_IOCTL_GET_VIDEOMODE, &tvmode)) == -1)
		check_errno("Cannot read tvmode");
	if((ioctl(DEV, EM8300_IOCTL_GET_ASPECTRATIO, &aspect)) == -1)
		check_errno("Cannot read aspect");
	if((ioctl(DEV, EM8300_IOCTL_GET_AUDIOMODE, &audio)) == -1)
		check_errno("Cannot read audio");
	if((ioctl(DEV, EM8300_IOCTL_GET_SPUMODE, &spu)) == -1)
		check_errno("Cannot read spu");

	printf("Video\tAspect\tAudio\tSpu\t\n");
	switch(tvmode) {
		case EM8300_VIDEOMODE_PAL:
			printf("PAL\t");
			break;
		case EM8300_VIDEOMODE_PAL60:
			printf("PAL60\t");
			break;
		case EM8300_VIDEOMODE_NTSC:
			printf("NTSC\t");
			break;
		default:
			printf("???\t");
	}
	switch(aspect) {
		case EM8300_ASPECTRATIO_4_3:
			printf("4:3\t");
			break;
		case EM8300_ASPECTRATIO_16_9:
			printf("16:9\t");
			break;
		default:
			printf("???\t");
	}
	switch(audio) {
		case  EM8300_AUDIOMODE_ANALOG:
			printf("ANALOG\t");
			break;
		case  EM8300_AUDIOMODE_DIGITALPCM:
			printf("PCM\t");
			break;
		case  EM8300_AUDIOMODE_DIGITALAC3:
			printf("AC3\t");
			break;
		default:
			printf("???\t");
	}
	switch(spu) {
		case  EM8300_SPUMODE_OFF:
			printf("OFF\n");
			break;
		case  EM8300_SPUMODE_ON:
			printf("ON\n");
			break;
		default:
			printf("???\n");
	}
	return 0;
}


int main(int argc, char * argv[]) {

	int UCODE; /* file descriptor for ucode file */
	int DEV; /* file descriptor for em8300 device*/
	struct stat s;
	em8300_microcode_t em8300_microcode;
	int opened_one,i,optindex;
	char * opt;
	char ucode_file[200]; // bad hardcoded value ;-)

	/* Vars to hold desired setings - init to -1 so we can see which were requested to be set explicitly by the user and which should be left alone */
        int tvmode=-1, aspect=-1, audio=-1, spu=-1, upload=0, display_only=0;


	char * devs[] = {"/dev/em8300-0","/dev/em8300-1","/dev/em8300-2","/dev/em8300-3"};
	int number_of_devs=4;

	/* Populate with default location - user can override below. */
	sprintf(ucode_file,"/usr/share/misc/em8300.uc"); 
	/* Read microcode file */
	if (argc == 1) {
	    /* behave as em8300init with no arguments */
	    /* just upload the microcode from the default location and exit */
	    upload=1;

	} else {

		/* parse the command line arguments here */
		optindex = 1;
		while (optindex < argc) {
			        opt = argv[optindex++];
				if (opt[0] == '-' && opt[1] != '\0') {
					switch(opt[1]) {
						case 'p':
							if(opt[2]=='6')
								tvmode = EM8300_VIDEOMODE_PAL60;
							else
								tvmode = EM8300_VIDEOMODE_PAL;
							break;
						case 'n':
							tvmode = EM8300_VIDEOMODE_NTSC;
							break;
						case 'd':
							audio = EM8300_AUDIOMODE_DIGITALPCM;
							break;
						case 'a':
							audio = EM8300_AUDIOMODE_ANALOG;
							break;
						case '3':
							audio = EM8300_AUDIOMODE_DIGITALAC3;
							break;
						case 'w':
							aspect = EM8300_ASPECTRATIO_16_9;
							break;
						case 'o':
							aspect = EM8300_ASPECTRATIO_4_3;
							break;
						case 'S':
							spu = EM8300_SPUMODE_ON;
							break;
						case 's':
							spu = EM8300_SPUMODE_OFF;
							break;
						case 'q':
							display_only=1;
							break;
						case 'f':
							if(opt[2] == '\0')
								sprintf(ucode_file,"%s", argv[optindex++]);
							else
								sprintf(ucode_file,"%s",opt+2);
							upload=1;
							printf("Using microcode file %s\n",ucode_file);
							break;
						 default:
							printf("Unknown option -%c \n\n",opt[1]);
							printf("Usage: em8300setup [-q]|[all other options]\n\nWhere options are one of the following (latter options will override previously\nspecified options for the same control):\n\n");
							printf("  -p, -p6, -n\tSet display mode to pal, pal60, ntsc\n");
							printf("  -a, -d, -3\tSet audio mode to analog, digitalpcm, digital ac3\n");
							printf("  -o, -w\tSet aspect ratio to normal[4:3], widescreen[16:9]\n");
							printf("  -S, -s\tSet spu mode On(S), Off(s)\n");
							printf("  -f <filename>\tSpecify alternate location of microcode\n\t\t(Defaults to /usr/share/misc/em8300.uc)\n");
							printf("  -q\t\tQuery the current settings for all of the above and\n\t\texit without making any changes\n");
							printf("  <none>\tPassing no option (except -f) causes the microcode\n\t\tto be loaded\n");

							exit(1);

					}



				}



		}

	}
	if(upload && !display_only) {
		UCODE = open(ucode_file,O_RDONLY);
		if(UCODE <0) {
			printf("Unable to open microcode file \"%s\" for reading\n", ucode_file);
			exit(1);
		}
	
		if(fstat(UCODE, &s ) <0) {
			printf("Unable to fstat ucode file\n");
			exit(1);
		}

		/* Alloc space for microcode and length structure */
	
		em8300_microcode.ucode = (char*) malloc(s.st_size * sizeof(char) );
		if (em8300_microcode.ucode == NULL) {
			printf("Unable to malloc() space for ucode\n");
			exit(1);
		}
		if ((i=read(UCODE,em8300_microcode.ucode,s.st_size)) < 1) {
			printf("Unable to read data from microcode file\n");
		}
	
		close(UCODE);
	}

	/* Open the device */

	opened_one = 0;

	for (i=0 ; i<number_of_devs; i++) {
		if ((DEV=open(devs[i],O_RDWR))== -1) {
			if (!opened_one) {
				printf("Can't open %s\n",devs[i]);
				exit(1);
			}
			/* exit normally if at least one card has been initialized */
			exit(0);
		}
		opened_one = 1;

		if(upload && !display_only) {
			/* Prepare ioctl */
			em8300_microcode.ucode_size = s.st_size;
	
			if(ioctl(DEV, EM8300_IOCTL_INIT, &em8300_microcode) == -1) {
				printf("Microcode upload to %s failed: \n",devs[i]);
				close(DEV);
				exit(1);
			}

			printf("Microcode uploaded to %s\n",devs[i]);

		}

		if(display_only) {
			display_current_settings(DEV);
			close(DEV);
			exit(0);
		}

		if(tvmode!=-1) {
			printf("Setting tvmode = %i\n",tvmode);
			if(ioctl(DEV, EM8300_IOCTL_SET_VIDEOMODE, &tvmode) == -1) 
				check_errno("Unable to set videomode");
		}
		if(aspect!=-1) {
			printf("Setting aspect = %i\n",aspect);
			if(ioctl(DEV, EM8300_IOCTL_SET_ASPECTRATIO, &aspect) == -1) 
				check_errno("Unable to set aspect ratio");
		}
		if(audio!=-1) {
			printf("Setting audio = %i\n",audio);
			if(ioctl(DEV, EM8300_IOCTL_SET_AUDIOMODE, &audio) == -1) 
				check_errno("Unable to set audio mode");
		}
		if(spu!=-1) {
			printf("Setting spu = %i\n",spu);
			if(ioctl(DEV, EM8300_IOCTL_SET_SPUMODE, &spu) == -1) 
				check_errno("Unable to set spu mode");
		}
		printf("Current settings are:\n");
		display_current_settings(DEV);
		 
		

		close(DEV);
	}
	exit(0);

}
