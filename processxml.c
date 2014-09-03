/********************************************************************
This file is part of ScriptInterpreter.
https://github.com/thfi/ScriptInterpreter

See file "AUTHORS" for a list of copyright holders.

ScriptInterpreter is free software: you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

ScriptInterpreter is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with ScriptInterpreter.
If not, see <http://www.gnu.org/licenses/>.
********************************************************************/

#include <stdio.h>
#include <string.h>

#include <expat.h>

#define BUFFER_SIZE 16384

int debug_output;

struct xmldata {
    int inside_timestep, timestep_is_empty;
    double timestep_delay, accumulated_delay;
    FILE *outputfile;
    char buffered_text_tags[BUFFER_SIZE];
    int buffered_text_tags_len;
};

void start_element(void *rawdata, const char *name, const char **attr) {
    struct xmldata *data = (struct xmldata *)rawdata;

    if (strcmp(name, "timestep") == 0) {
        data->inside_timestep = 1;
        data->timestep_is_empty = 1;

        data->timestep_delay = 0.0;
        for (int i = 0; attr[i] != NULL; i += 2)
            if (strcmp(attr[i], "delay") == 0) {
                data->timestep_delay = atof(attr[i + 1]);
                break;
            }
    } else if (data->inside_timestep) { /// tag name is not "timestep" as tested already
        data->timestep_is_empty = 0;
    }

    if (strcmp(name, "text") == 0 || strcmp(name, "newline") == 0) {
        char textbuffer[BUFFER_SIZE];
        int maxlen = BUFFER_SIZE;
        maxlen -= snprintf(textbuffer, maxlen, "<%s", name);
        for (int i = 0; attr[i]; i += 2)
            maxlen -= snprintf(textbuffer, maxlen, " %s=\"%s\"", attr[i], attr[i + 1]);
        maxlen -= snprintf(textbuffer, maxlen, ">");
// TODO
//        maxlen -= snprintf(textbuffer, maxlen, "</%s>", name);
    }

    printf("%s %d", name, data->inside_timestep);

    for (int i = 0; attr[i]; i += 2) {
        printf(" %s=\"%s\"", attr[i], attr[i + 1]);
    }

    printf("\n");
}

void end_element(void *rawdata, const char *name) {
    struct xmldata *data = (struct xmldata *)rawdata;
    if (strcmp(name, "timestep") == 0) {
        data->inside_timestep = 0;
        if (data->timestep_is_empty && debug_output)
            fprintf(stderr, "empty timestep, delay was =%.3lf\n", data->timestep_delay);
    }
}


int main(int argc, char *argv[])
{
    debug_output = 0;
    char *inputfilename, *outputfilename;
    FILE *inputfile, *outputfile;

    struct xmldata data;

    inputfilename = NULL;
    if (argc > 1 && strcmp("--debug", argv[1]) == 0) {
        fprintf(stderr, "Enabling debug output\n");
        debug_output = 1;
    }

    if ((argc > 2 && debug_output == 0) || argc > 3) {
        inputfilename = argv[argc - 2];
        if (debug_output) fprintf(stderr, "Reading XML from file \"%s\"\n", inputfilename);
    } else if (debug_output)
        fprintf(stderr, "Reading XML from stdin\n");

    if ((argc > 3 && debug_output == 0) || argc > 4) {
        outputfilename = argv[argc - 1];
        if (debug_output) fprintf(stderr, "Writing XML to file \"%s\"\n", outputfilename);
    } else if (debug_output)
        fprintf(stderr, "Writing XML to stdout\n");

    if (inputfilename == NULL)
        inputfile = stdin;
    else {
        inputfile = fopen(inputfilename, "r");
        if (!inputfile) {
            fprintf(stderr, "Cannot open inputfilename \"%s\"\n", inputfilename);
            return 2;
        }
    }

    if (outputfilename == NULL)
        outputfile = stdout;
    else {
        outputfile = fopen(outputfilename, "w");
        if (!outputfile) {
            if (inputfile != stdin)
                fclose(inputfile);
            fprintf(stderr, "Cannot open outputfilename \"%s\"\n", outputfilename);
            return 3;
        }
    }

    XML_Parser p = XML_ParserCreate(NULL);
    if (p == NULL) {
        fprintf(stderr, "Couldn't allocate memory for parser\n");
        return 1;
    }

    XML_SetElementHandler(p, start_element, end_element);

    XML_SetUserData(p, &data);
    data.inside_timestep = 5;
    data.outputfile = outputfile;
    data.accumulated_delay = 0.0;
    data.buffered_text_tags_len = 0;

    char buffer[BUFFER_SIZE];
    int done = 0;
    while (done == 0) {
        int len = fread(buffer, 1, BUFFER_SIZE, inputfile);
        if (ferror(inputfile)) {
            fprintf(stderr, "Read error from input file\n");
            return 2;
        }
        int done = feof(stdin);

        if (!XML_Parse(p, buffer, len, done)) {
            int errorcode = XML_GetErrorCode(p);
            if (errorcode == 36) ///< end of file?
                break;
            fprintf(stderr, "Parse error at line %ld:\n%s (code=%d)\n", XML_GetCurrentLineNumber(p), XML_ErrorString(errorcode), errorcode);
            return 1;
        }
    }

    return 0;
}
