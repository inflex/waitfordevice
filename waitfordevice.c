/**
 * waitfordevice
 *
 * AUTHOR: Paul L Daniels ( pldaniels@pldaniels.com )
 * DATE: 21/06/2014
 * 
 * DESCRIPTION: 
 *	Created to help with data recovery using ddrescue and an external USB power switch. 
 *	WFD will stall the execution of a recovery script until such time that the external 
 *	drive being recovered has come back on line and is ready to be rescued again.
 *
 *	A typical script would be something like the following
 -----------------------
#!/bin/sh

while [ 1 -eq 1 ]; do
./powerSwitch on 3
./waitfordevice -d /dev/sde 
sleep 2
~/ddrpld -X -v -f /dev/sde drive.img drive.log
./powerSwitch off 3
echo "Sleeping..."
sleep 60
done
------------------------
 *	
 * Very useful for software-only recoveries of drives that don't want to stay alive for
 * more than a short time in various parts of the recovery process.  Typically you'll 
 * recognise these drives by their very large error size when initially running ddrescue
 * ie;  40MB recovered, 499GB error, and the error happens very quickly.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <linux/hdreg.h>

#define EXIT_NORMAL 0
#define EXIT_ERROR 1
#define EXIT_SIGNATURE_MISSMATCH 2
#define EXIT_GIVEUP 3

int main( int argc, char **argv )
{
	unsigned char *hdsn;
	int i;
	int print_model = 0;
	int print_serial = 0;
	int verbose = 0;
	int polling_wait = 250000;
	char *sig = NULL;
	char *device = NULL;
	int retries = -1;
	int parameter_limit;

	int e, fd;
	static struct hd_driveid hd;

	if ( argc < 2 ) {
		fprintf(stdout,"%s -d <filename or device to monitor for> [-v] [-M (print model)] [-S (print serial)] [-s <signature>] [-p <polling time (milliseconds)>] [-r <retries>]\n", argv[0]);
		exit (EXIT_ERROR);
	}


	parameter_limit = argc -1;

	for (i = 1; i < argc; i++ ) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {

				case 'v':
					verbose = 1;
					break;

				case 'S':
					print_serial = 1;
					break;

				case 'M':
					print_model = 1;
					break;

				case 'd':
					if (i >= parameter_limit) { fprintf(stderr,"Insufficient parameters\n"); exit(EXIT_ERROR); }
					device = argv[++i];
					break;

				case 's':
					if (i >= parameter_limit) { fprintf(stderr,"Insufficient parameters\n"); exit(EXIT_ERROR); }
					sig = argv[++i];
					break;

				case 'p':
					if (i >= parameter_limit) { fprintf(stderr,"Insufficient parameters\n"); exit(EXIT_ERROR); }
					polling_wait = strtol(argv[++i],NULL,10) *1000;
					break;

				case 'r':
					if (i >= parameter_limit) { fprintf(stderr,"Insufficient parameters\n"); exit(EXIT_ERROR); }
					retries = strtol(argv[++i],NULL,10);
					break;

				default:
					fprintf(stderr,"Unknown parameter '%s'\n", argv[i]);
					exit(EXIT_ERROR);
			}
		} // IF
	} // FOR


	if (verbose) {
		fprintf(stdout,"Waiting for '%s'",device);
		if (sig) fprintf(stdout," with signature: '%s'", sig);
		fprintf(stdout," polling every %dms\n", polling_wait /1000);
	}

	while (1) {
		e = access( device, F_OK );
		if (e == 0) break;
		if (verbose) { fprintf(stdout,"."); fflush(stdout); }

		if ( retries > 0 ) {
			if ( retries == 1 ) {
				fprintf(stdout,"No device found\n");
				exit(EXIT_GIVEUP);
			} else retries--;
		}

		usleep(polling_wait);
	}

	if ((fd = open(device, O_RDONLY|O_NONBLOCK)) < 0) {
		if (verbose) fprintf(stdout,"ERROR: Cannot open device %s\n", device);
		exit(EXIT_ERROR);
	}

	if (!ioctl(fd, HDIO_GET_IDENTITY, &hd)) {

		if (print_model) {
			hdsn = hd.model;
			while (*hdsn) { if (isalnum(*hdsn)) { fprintf(stdout,"%c", *hdsn); } hdsn++; }
			if (!print_serial) fprintf(stdout,"\n"); fflush(stdout);
		}

		if (print_serial) {
			if (print_model) fprintf(stdout,"-");
			hdsn = hd.serial_no;
			while (*hdsn) { if (isalnum(*hdsn)) { fprintf(stdout,"%c", *hdsn); } hdsn++; }
			fprintf(stdout,"\n"); fflush(stdout);
		}


		if (verbose) fprintf(stdout,"%s %s\n", hd.model, hd.serial_no);

		if (sig) {
			if ((strstr((char *)hd.model, sig) == NULL)&&(strstr((char *)hd.serial_no, sig) == NULL)) {
				if (verbose) fprintf(stdout,"No signature match on device\n");
				exit(EXIT_SIGNATURE_MISSMATCH);
			}

			if (verbose) fprintf(stdout,"Signature match\n");
		}

	} else if (errno == -ENOMSG) {
		if (sig) {
			if (verbose) fprintf(stdout,"\nNo hard disk identification information available\n");
			exit(EXIT_SIGNATURE_MISSMATCH);
		}
	} else {
		if (verbose) fprintf(stdout,"ERROR: HDIO_GET_IDENTITY");
	}

		fflush(stdout);

	return EXIT_NORMAL;
}
/** END **/
