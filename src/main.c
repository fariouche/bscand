/*
   Copyright 2017 <fariouche@yahoo.fr>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sane/sane.h>
#include <sane/saneopts.h>
#include <cups/cups.h>
#include <fcntl.h>
#include <semaphore.h>
#include "process.h"
#include "stiff.h"

#define STRIP_HEIGHT	256

typedef struct
{
  uint8_t *data;
  int width;    /*WARNING: this is in bytes, get pixel width from param*/
  int height;
  int x;
  int y;
}
Image;

static SANE_Handle device = NULL;

static int scan_resolution_idx = -1;
static int scan_mode_idx = -1;

static int append_to_tiff(const char* in_tiff, const char* out_tiff)
{
	pid_t pid;
	const char* args[7];
	int ret = -1;

	args[0] = "/usr/bin/tiffcp";
	args[1] = "-a";
	args[2] = "-c";
	args[3] = "lzw";
	args[4] = in_tiff;
	args[5] = out_tiff;
	args[6] = NULL;

	pid = exec_process(args[0], args);
	if(pid)
	{
		ret = wait_process(pid);
		if(ret)
			fprintf(stderr, "tiffcp returned an error %d\n", ret);
	}

	return ret;
}

static int append_to_pdf(const char* in_tiff, const char* out_pdf)
{
	pid_t pid;
	const char* args[7];
	int ret = -1;

	args[0] = "/usr/bin/tiff2pdf";
	args[1] = "-z";
	args[2] = "-o";
	args[3] = out_pdf;
	args[4] = in_tiff;
	args[5] = NULL;


	pid = exec_process(args[0], args);
	if(pid)
	{
		ret = wait_process(pid);
		if(ret)
			fprintf(stderr, "tiff2pdf returned an error %d\n", ret);
	}

	return ret;
}

static int print_pdf(const char* pdf)
{
	char* instance;
	cups_dest_t	*dest;
	int j;
	int	num_options = 0;
	int job_id;
	cups_option_t	*options = NULL;
	const char	*files[1];

	if(access(pdf, R_OK) != 0)
	{
		fprintf(stderr, "Error - unable to access '%s' - %s\n", pdf, strerror(errno));
		return 1;
	}


/*	const char* printer = "Canon_LBP7660C";
	dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, printer, instance);*/
	dest = cupsGetNamedDest(NULL, NULL, NULL);

	if(dest != NULL)
	{
		for (j = 0; j < dest->num_options; j++)
		{
		  if(cupsGetOption(dest->options[j].name, num_options, options) == NULL)
		    num_options = cupsAddOption(dest->options[j].name, dest->options[j].value, num_options, &options);
		}
	}
	else if (cupsLastError() == IPP_STATUS_ERROR_BAD_REQUEST ||
		       cupsLastError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
	{
		fprintf(stderr, "cannot find printer\n");
		return 1;
	}
	files[0] = pdf;
	job_id = cupsPrintFiles(dest->name, 1, files, "bscand job", num_options, options);
	if (job_id < 1)
  {
    fprintf(stderr, "%s\n", cupsLastErrorString());
    return 1;
	}

	cupsFreeOptions(num_options, options);

	return 0;
}

static void *advance(Image * image)
{
	if(++image->x >= image->width)
	{
		image->x = 0;
		if(++image->y >= image->height || !image->data)
		{
			size_t old_size = 0, new_size;

			if(image->data)
				old_size = image->height * image->width;

			image->height += STRIP_HEIGHT;
			new_size = image->height * image->width;

			if(image->data)
				image->data = realloc (image->data, new_size);
			else
				image->data = malloc (new_size);
			if(image->data)
				memset (image->data + old_size, 0, new_size - old_size);
		}
	}
	if (!image->data)
		fprintf (stderr, "can't allocate image buffer (%dx%d)\n", image->width, image->height);

	return image->data;
}


static int do_scan(const char* out_tiff, int resolution_value, const char* mode)
{
	SANE_Status status;
	int first_frame = 1;
	int must_buffer = 0;
	int i;
	int offset = 0;
	int len;
	SANE_Byte *buffer = NULL;
	size_t buffer_size;
	Image image = { 0, 0, 0, 0, 0 };
	SANE_Parameters parm;
	FILE* ofp = NULL;
	SANE_Int info = 0;
	int ret = 0;
	char tmp_mode[8]={0};

	status = sane_control_option(device, scan_resolution_idx, SANE_ACTION_SET_VALUE, &resolution_value, &info);
	if(status != SANE_STATUS_GOOD)
	{
		fprintf (stderr, "setting of option resolution failed (%s)\n", sane_strstatus(status));
    return 1;
  }

	strcpy(tmp_mode, mode);
	status = sane_control_option(device, scan_mode_idx, SANE_ACTION_SET_VALUE, tmp_mode, &info);
	if(status != SANE_STATUS_GOOD)
	{
		fprintf (stderr, "setting of option mode failed (%s)\n", sane_strstatus(status));
    return 1;
  }

	status = sane_start (device);
	if(status != SANE_STATUS_GOOD)
	{
		fprintf(stderr, "sane_start: %s\n", sane_strstatus(status));
	  return 1;
	}
	
	status = sane_get_parameters(device, &parm);
	if(status != SANE_STATUS_GOOD)
	{
		fprintf (stderr, "sane_get_parameters: %s\n", sane_strstatus(status));
	  ret = 1;
		goto ret_err;
	}
	ofp = fopen(out_tiff, "wb");
	if(!ofp)
	{
		fprintf(stderr, "cannot create file '%s' (%s)\n", out_tiff, strerror(errno));
		ret = 1;
		goto ret_err;
	}

	buffer_size = (32 * 1024);
	buffer = malloc(buffer_size);
	do
	{
		if(first_frame)
		{
			switch(parm.format)
			{
				case SANE_FRAME_RED:
				case SANE_FRAME_GREEN:
				case SANE_FRAME_BLUE:
					if(parm.depth != 8)
						fprintf(stderr, "expected 8 bit depth!!\n");
					must_buffer = 1;
					offset = parm.format - SANE_FRAME_RED;
				break;

				case SANE_FRAME_RGB:
					if((parm.depth != 8) && (parm.depth != 16))
						fprintf(stderr, "expected 8 or 16 bits!!\n");
				case SANE_FRAME_GRAY:
					//assert ((parm.depth == 1) || (parm.depth == 8) || (parm.depth == 16));
					if(parm.lines < 0)
					{
						must_buffer = 1;
						offset = 0;
					}
					else
					{
						sanei_write_tiff_header(parm.format, parm.pixels_per_line, parm.lines, parm.depth, resolution_value, NULL, ofp);
			  	}
				break;

				default:
					break;
			}

			if(must_buffer)
			{
				image.width = parm.bytes_per_line;

				if (parm.lines >= 0)
					image.height = parm.lines - STRIP_HEIGHT + 1;
				else
					image.height = 0;

				image.x = image.width - 1;
				image.y = -1;
				if(!advance (&image))
				{
					status = SANE_STATUS_NO_MEM;
					ret = 1;
					goto ret_err;
				}
			}
		}
		else
		{
			if((parm.format < SANE_FRAME_RED) || (parm.format > SANE_FRAME_BLUE))
				fprintf(stderr, "format out of range (%d)\n", parm.format);
			offset = parm.format - SANE_FRAME_RED;
			image.x = image.y = 0;
		}
	
		while(1)
		{
			status = sane_read (device, buffer, buffer_size, &len);
			if (status != SANE_STATUS_GOOD)
			{
				if (status != SANE_STATUS_EOF)
				{
					fprintf(stderr, "sane_read: %s\n", sane_strstatus(status));
					ret = status;
					goto ret_err;
				}
				break;
			}

			if (must_buffer)
			{
				switch (parm.format)
				{
					case SANE_FRAME_RED:
					case SANE_FRAME_GREEN:
					case SANE_FRAME_BLUE:
						for(i = 0; i < len; ++i)
						{
							image.data[offset + 3 * i] = buffer[i];
							if(!advance (&image))
							{
								ret = SANE_STATUS_NO_MEM;
								goto ret_err;
							}
						}
						offset += 3 * len;
					break;

					case SANE_FRAME_RGB:
						for(i = 0; i < len; ++i)
						{
							image.data[offset + i] = buffer[i];
							if(!advance (&image))
							{
								ret = SANE_STATUS_NO_MEM;
								goto ret_err;
							}
						}
		  			offset += len;
		  		break;

					case SANE_FRAME_GRAY:
						for(i = 0; i < len; ++i)
						{
							image.data[offset + i] = buffer[i];
							if(!advance (&image))
							{
								ret = SANE_STATUS_NO_MEM;
								goto ret_err;
							}
						}
						offset += len;
					break;

					default:
		  		break;
				}
	    }
	  	else			/* ! must_buffer */
	    {
				fwrite(buffer, 1, len, ofp);
			}
		}
		first_frame = 0;
	} while (!parm.last_frame);
	

	if(must_buffer)
	{
		image.height = image.y;

		sanei_write_tiff_header(parm.format, parm.pixels_per_line, image.height, parm.depth, resolution_value, NULL, ofp);
		fwrite(image.data, 1, image.height * image.width, ofp);
	}

ret_err:
	if (image.data)
		free (image.data);
	if(buffer)
		free(buffer);
	if(ofp)
		fclose(ofp);
	//if(ret)
		sane_cancel(device);
/*
  if(!sane_get_option_descriptor(device, 0))
	{
	  fprintf (stderr, "unable to get option count descriptor\n");
	}

	status = sane_control_option (device, 0, SANE_ACTION_GET_VALUE, &info, 0);
	if (status != SANE_STATUS_GOOD)
	{
		fprintf (stderr, "Could not get value for option 0: %s\n", sane_strstatus(status));
	}
*/
	return ret;
}

static void do_action(const char* button_name)
{
	
	if(strcmp(button_name, "file") == 0)
	{
		do_scan("/tmp/bscand.out.tiff", 300, SANE_VALUE_SCAN_MODE_COLOR);
		append_to_tiff("/tmp/bscand.out.tiff", "/tmp/bscand_tmp.out.tiff");
	}
	else if(strcmp(button_name, "extra") == 0)
	{
		if(access("/tmp/bscand_tmp.out.tiff", F_OK) == 0)
		{
			append_to_pdf("/tmp/bscand_tmp.out.tiff", "/tmp/scan.pdf");
			unlink("/tmp/bscand_tmp.out.tiff");
		}
	}
	else if(strcmp(button_name, "copy") == 0)
	{
		if(access("/tmp/scan.pdf", F_OK))
		{
			do_scan("/tmp/bscand.out.tiff", 300, SANE_VALUE_SCAN_MODE_COLOR /*SANE_VALUE_SCAN_MODE_GRAY*/);
			append_to_pdf("/tmp/bscand.out.tiff", "/tmp/scan.pdf");
		}
		print_pdf("/tmp/scan.pdf");
		unlink("/tmp/bscand_tmp.out.tiff");
		unlink("/tmp/scan.pdf");
	}
	else if(strcmp(button_name, "email") == 0)
	{
	}
	else if(strcmp(button_name, "scan") == 0)
	{
	}
	else
	{
		printf("No action for button '%s'\n", button_name);
	}

}

static void scan_buttons(const char* devname)
{
	SANE_Status status;
	const SANE_Option_Descriptor *opt;
	struct option *all_options;
	int option_number_len;
	int *option_number;
	SANE_Int num_dev_options;
	int i;
	int num_buttons;
  int* buttons;
	int b;
	char* tmp_name;
	sem_t* sem;
	int ret;

	status = sane_open (devname, &device);
  if(status != SANE_STATUS_GOOD)
  {
		fprintf(stderr, "canot open sane device '%s' - (%s)\n", devname, sane_strstatus(status));
		return;
	}

  opt = sane_get_option_descriptor(device, 0);
  if(!opt)
	{
	  fprintf (stderr, "unable to get option count descriptor\n");
	  sane_close(device);
		return;
	}

	status = sane_control_option (device, 0, SANE_ACTION_GET_VALUE, &num_dev_options, 0);
	if (status != SANE_STATUS_GOOD)
	{
		fprintf (stderr, "Could not get value for option 0: %s\n", sane_strstatus(status));
    sane_close(device);
		return;
	}

	num_buttons = 0;
	for (i = 1; i < num_dev_options; ++i)
	{
		opt = sane_get_option_descriptor(device, i);
		if (opt == NULL)
		{
			fprintf (stderr, "Could not get option descriptor for option %d\n",i);
			//sane_close(device);
			//return;
			continue;
		}
		if(SANE_OPTION_IS_ACTIVE(opt->cap) && (opt->cap & SANE_CAP_HARD_SELECT))
		{
			printf("found button '%s'\n", opt->name);
			num_buttons++;
		}
		else if((opt->type == SANE_TYPE_STRING) &&
						(strcmp (opt->name, SANE_NAME_SCAN_MODE) == 0))
		{
			scan_mode_idx = i;
		}
		else if((opt->type == SANE_TYPE_FIXED || opt->type == SANE_TYPE_INT) &&
						(opt->size == sizeof (SANE_Int)) &&
						(opt->unit == SANE_UNIT_DPI) &&
						(strcmp (opt->name, SANE_NAME_SCAN_RESOLUTION) == 0))
		{
			scan_resolution_idx = i;
		}
		
	}
	if(num_buttons == 0)
	{
		fprintf(stderr, "no buttons found\n");
		sane_close(device);
		return;
	}
	if(scan_resolution_idx == -1)
	{
		fprintf(stderr, "No scan resolution found!");
		sane_close(device);
		return;
	}
	if(scan_mode_idx == -1)
	{
		fprintf(stderr, "No scan mode found!");
		sane_close(device);
		return;
	}
	printf("found %d buttons\n", num_buttons);
	buttons = (int*)malloc(num_buttons * sizeof(*buttons));
	memset(buttons, 0, num_buttons * sizeof(*buttons));

	tmp_name = malloc(strlen(devname)+strlen(".open")+1);
	strcpy(tmp_name, devname+strlen("net:[::1]:"));
//sem_unlink(tmp_name);
	strcat(tmp_name, ".open");
//sem_unlink(tmp_name);
//exit(0);
	sem = sem_open(tmp_name, O_CREAT, 0700, 0);
	if(sem == SEM_FAILED)
		fprintf(stderr, "process_request: cannot open or create semaphore `%s' - %s\n",tmp_name, strerror(errno));
	free(tmp_name);

	sem_trywait(sem);/* decrement our count*/

	while(1)
	{
		usleep(200000);

		while(1)
		{
			ret = sem_trywait(sem);
			if(ret < 0)
			{
				if(errno != EAGAIN)
				{
					fprintf(stderr, "error waiting for semaphore (%s)\n", strerror(errno));
				}

			
				if(device == NULL)
				{
					printf("device should be free now, opening...\n");
					status = sane_open(devname, &device);
					sem_trywait(sem);/* decrement our count*/
					if(status != SANE_STATUS_GOOD)
					{
						fprintf(stderr, "canot open sane device '%s' - (%s)\n", devname, sane_strstatus(status));
						continue;
					}
					
				}
				break;
			}
			else
			{
				printf("sem ret = %d\n", ret);
				fprintf(stderr, "someone is accessing the scanner, wait a bit\n");
				sane_close(device);
				device = NULL;
				usleep(2000000);
				continue;
			}
		}

		b = 0;
		for (i = 1; i < num_dev_options; ++i)
		{
			opt = sane_get_option_descriptor(device, i);
			if (opt == NULL)
			{
				fprintf (stderr, "Could not get option descriptor for option %d\n",i);
				sane_close(device);
				return;
			}
			if(SANE_OPTION_IS_ACTIVE(opt->cap) && (opt->cap & SANE_CAP_HARD_SELECT))
			{
				int val = 0;
				if(opt->size != 4)
				{
					printf("Invalid size, expected 4 for a button, got %d\n", opt->size);
					break;
				}
				sane_control_option (device, i, SANE_ACTION_GET_VALUE, &val, 0);
				if((buttons[b] != val) && (val == 0))
				{
					printf("button '%s' released\n", opt->name);
					do_action(opt->name);
				}
				buttons[b] = val;
				b++;
			}
		}
	}
	sem_close(sem);
	sane_close(device);
}

static void auth_callback( SANE_String_Const res,
	                         SANE_Char *username,
	                         SANE_Char *password)
{
	printf("Auth callback not implemented!!\n");
	memset (username, 0, SANE_MAX_USERNAME_LEN);
	memset (password, 0, SANE_MAX_PASSWORD_LEN);
}

static void sighandler(int signum)
{
	static SANE_Bool first_time = SANE_TRUE;

	if (device)
	{
		fprintf (stderr, "received signal %d\n", signum);
		if (first_time)
		{
			first_time = SANE_FALSE;
			fprintf (stderr, "trying to stop scanner\n");
			sane_cancel(device);
			sane_close(device);
		}
		else
		{
			fprintf (stderr, "aborting\n");
		}
	}
	_exit (0);
}

int main(int argc, char** argv)
{
	SANE_Word be_version_code;
	SANE_Status status;
	const SANE_Device **device_list;
	int i;

	printf("starting...\n");

#ifdef SIGHUP
	signal(SIGHUP, sighandler);
#endif
#ifdef SIGPIPE
	signal(SIGPIPE, sighandler);
#endif
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);

	sane_init(&be_version_code, auth_callback);
	if(status != SANE_STATUS_GOOD)
	{
		fprintf(stderr, "failed to initialize sane (%s)\n", sane_strstatus (status));
		return -1;
	}
	
	status = sane_get_devices (&device_list, SANE_FALSE);
	if (status != SANE_STATUS_GOOD)
	{
		fprintf (stderr, "sane_get_devices() failed: %s\n", sane_strstatus (status));
		sane_exit();
		return -1;
	}

	for (i = 0; device_list[i]; ++i)
	{
		/*printf("device `%s' is a %s %s %s\n",
			        device_list[i]->name, device_list[i]->vendor,
			        device_list[i]->model, device_list[i]->type);*/
		if(strncmp("net:[::1]", device_list[i]->name, 9) == 0)
			break;
	}

	if(device_list[i] == NULL)
	{

		fprintf(stderr, "network sane server over ipv6 not found\n");
		sane_exit();
		return -1;
	}

	printf("Found device '%s'\n", device_list[i]->name);

	scan_buttons(device_list[i]->name);
	sane_exit();
	return 0;
}