#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "debug.h"

void usage(char *name)
{
	fprintf(stderr, "Usage:\n"
					"%s [ --help ] [ --option1 value ] [ --option2 value ] [ --debug file ] file\n\n"
					"Prints values given as option. The --debug flag enables the output produced by "
					"calls to the debug function in the named source file.\n",
			name);
}

void sample_function(char *option1, char *option2)
{
	debug("Beginning of the sample function\n");
	debug("Given values are [ %s ] and [ %s ], time to print them:\n", option1, option2);
	printf("Option 1: %s\n", option1);
	printf("Option 2: %s\n", option2);
	debug("End of the sample function\n");
}

int main(int argc, char *argv[])
{
	int opt;
	char *option1, *option2;

	struct option longopts[] = {
		{"debug", required_argument, NULL, 'd'},
		{"option1", required_argument, NULL, '1'},
		{"option2", required_argument, NULL, '2'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}};

	option1 = NULL;
	option2 = NULL;
	while ((opt = getopt_long(argc, argv, "1:2:d:h", longopts, NULL)) != -1)
	{
		switch (opt)
		{
		case '1':
			option1 = optarg;
			break;
		case '2':
			option2 = optarg;
			break;
		case 'h':
			usage(argv[0]);
			exit(0);
		case 'd':
			add_debug_to(optarg);
			break;
		default:
			fprintf(stderr, "Unrecognized option %c\n", opt);
			usage(argv[0]);
			exit(1);
		}
	}

	sample_function(option1, option2);
	return 0;
}
