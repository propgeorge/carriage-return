/* strips carriage returns from input to convert from DOS format
 * or places carriage returns before newlines to convert to DOS format
 * written by Alexander George
 * 11/20/2015
 */
#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

// struct to hold the flags generated from the command line parse
struct Flags
{
	int help;
	int reverse;
	int count;
};

int parse(int, char*[], struct Flags*);
int printhelp(void);
int process(char*, struct Flags*);
int getfd(char*, char**, FILE**, FILE**);
int stripcr(FILE*, FILE*);
int reverserc(FILE*, FILE*);
int closefiles(char*, char*, FILE*, FILE*);

int main(int argc, char *argv[])
{
	int numfiles, i, error;
	struct Flags f;

	numfiles = parse(argc, argv, &f);
	if (f.help)	// if the help flag is specified, print help and return
	{
		printhelp();
		return 0;
	}
	else if (numfiles < 0)	// if unknown command line option, return error
	{
		return -1;
	}
	else if (numfiles == 0) // if no files specified, use as filter
	{
		process(NULL, &f);
	}
	else if (numfiles)	// process each file according to flags
	{
		for (i = 1; i < argc; i++)
		{
			error = 0;
			if (argv[i][0] != '-')	// if it is a file, process it
			{
				error = process(argv[i], &f);
			}
			if (error)	// print an error message if an error occurs but continue processing remaining files
			{
				fprintf(stderr, "Error processing %s\n", argv[i]);
			}
		}
	}
	return 0;
}

// fill flags struct and return the number of files or error
int parse(int argc, char *argv[], struct Flags *f)
{
	int i, numfiles;

	f->help = f->reverse = f->count = numfiles = 0;
	for (i = 1; i < argc; i++)	// for each argument
	{
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-?") || !strcmp(argv[i], "--help"))
		{
			f->help = 1;
		}
		else if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--reverse"))
		{
			f->reverse = 1;
		}
		else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--count"))
		{
			f->count = 1;
		}
		else if (argv[i][0] == '-')	// error
		{
			fprintf(stderr, "Unknown option %s\n", argv[i]);
			return  -1;
		}
		else
		{
			numfiles++;
		}
	}
	return numfiles;	// the actual number of files doesn't matter; only whether it is zero or not
}

int printhelp(void)
{
	printf("usage: cr [option] ... [file] ...\nstrips carriage returns from input\noptions:\n");
	printf("%-20sConvert to DOS format by adding carriage returns before newlines\n", "-r, --reverse");
	printf("%-20sDisplay the number of carriage returns processed\n", "-c, --count");
	printf("%-20sShow this help message\n", "-h, -?, --help");
}

// process a single file given the flags
int process(char *file, struct Flags *f)
{
	int count, error;
	char *tempname;
	FILE *in, *out;

	if (file)
	{
		error = getfd(file, &tempname, &in, &out);
		if (error)
		{
			return error;
		}
	}
	else
	{
		in = stdin;
		out = stdout;
	}

#ifdef WIN32
	if (_setmode(_fileno( in), _O_BINARY) < 0 || _setmode(_fileno( out), _O_BINARY) < 0)
	{
		fprintf(stderr, "Error setting file mode to binary\n");
		return -1;
	}
#endif
	if (f->reverse)
	{
		count = reversecr(in, out);
	}
	else
	{
		count = stripcr(in, out);
	}
	if (f->count)
	{
		if (file)
		{
			printf("%s:\t", file);
		}
		printf("%d\n", count);
	}
	if (file)
	{
		closefiles(file, tempname, in, out);
	}
	return 0;
}

// get input and output file descriptors
int getfd(char *file, char **tempname, FILE **in, FILE **out)
{
	static char temp[] = "XXXXXX";
	// open input file
	*in = fopen(file, "rb");
	if (!*in)
	{
		fprintf(stderr, "Error opening file %s for input\n", file);
		return -1;
	}
	*tempname = mktemp(temp);
	*out = fopen(*tempname, "wb");
	if (!*out)
	{
		fprintf(stderr, "Error opening temporary output file\n");
		return -2;
	}
	return 0;
}

int stripcr(FILE *in, FILE *out)
{
	int c, count;

	count = 0;
	while ((c = fgetc(in)) != EOF)
	{
		if (c != '\r')
		{
			fputc(c, out);
		}
		else
		{
			count++;
		}
	}
	return count;
}

int reversecr(FILE *in, FILE *out)
{
	int c, d, count;

	count = d = 0;
	while ((c = fgetc(in)) != EOF)
	{
		if (c == '\n')
		{
			if (d != '\r')
			{
				fputc('\r', out);
				count++;
			}
		}
		fputc(c, out);
		d = c;
	}
	return count;
}

int closefiles(char *file, char *tempname, FILE *in, FILE *out)
{
	fclose(in);
	fclose(out);
	if (remove(file))
	{
		fprintf(stderr, "Error removing %s\n", file);
		return -3;
	}
	if (rename(tempname, file))
	{
		fprintf(stderr, "Error renaming file.\n");
		return -4;
	}
	return 0;
}
