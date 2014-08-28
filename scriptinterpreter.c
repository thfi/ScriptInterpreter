#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024
#define ARRAY_LENGTH 256

size_t typescriptbuffer_size;
char *typescriptbuffer;

FILE *timefile, *typescriptfile, *xmloutputfile;

/**
 * For a given integer number n, return
 * - 1 if n is zero or negative
 * - n itself if n is a power of 2
 * - the next power of 2 larger than n
 */
int roundup_powerof2(int n)
{
    int result = 1;
    while (n > 0) {
        n /= 2;
        result *= 2;
    }
    return result;
}

/**
 * Continue reading from an input file,
 * discarding all read data until
 * - there is a line break denoted by \n
 * - the end of file is reached
 */
void skipline(FILE *input)
{
    int c;
    while ((c = fgetc(input)) != EOF && c != '\n');
}

/// Handle XML entities correctly
void xmlized_print(FILE *output, char c)
{
    switch (c) {
    case '<':
        fprintf(output, "&lt;");
        break;
    case '>':
        fprintf(output, "&gt;");
        break;
    case '&':
        fprintf(output, "&amp;");
        break;
    default:
        fprintf(output, "%c", c);
    }
}

/**
 * Convert a sequence of characters into an integer number.
 * The sequence is limited by 'len' or the first appearance
 * of a (semi)colon or the null character.
 * The function will convert a valid sequence like '6721' to
 * the numeric value 6721.
 * The function will return -1 if the sequence contains an
 * invalid character like 'a' or 'x'.
 */
int ascii_to_dec(char *sequence, int *len)
{
    int result = 0;
    int left = *len;
    *len = 0;
    while (left > 0) {
        if (*sequence == 0 || *sequence == 0x3b /* semicolon */ || *sequence == 0x3a /* colon */)
            break;
        if (*sequence >= 0x3c && *sequence <= 0x3f) {
            fprintf(stderr, "Parameter string contains 'future use' bits\n");
            return -1;
        }
        if (*sequence < 0x30 /* '0' */ || *sequence > 0x39 /* '9' */) {
            fprintf(stderr, "Not a valid number char in sequence\n");
            return -1;
        }
        result *= 10;
        result += *sequence - '0';
        ++sequence;
        *len += 1;
        --left;
    }
    return result;
}

int parameterstring_to_intarray(char *arraystring, int len, int *array, int arraylen)
{
    int arraypos = 0;
    while (len > 0 && arraypos < arraylen) {
        int declen = len;
        int dec = ascii_to_dec(arraystring, &declen);
        if (dec < 0) return dec;
        arraystring += declen;
        array[arraypos++] = dec;
        if (*arraystring != ';') break;
        ++arraystring;
    }

    if (arraypos < arraylen)
        array[arraypos] = -1;

    return arraypos;
}

/**
 * The name (as a string) of a color corresponding to its
 * numeric value will be written into the result string.
 * Example strings written to the result string include
 * 'red' or 'blue'. For valid color codes, the function
 * will return 0. For invalid color codes, the return
 * value is 1 and the text 'unknown' will be written
 * into the result string.
 */
int colortostring(int color, char *result, int resultlen)
{
    switch (color % 10) {
    case 0:
        snprintf(result, resultlen, "black");
        return 0;
    case 1:
        snprintf(result, resultlen, "red");
        return 0;
    case 2:
        snprintf(result, resultlen, "green");
        return 0;
    case 3:
        snprintf(result, resultlen, "yellow");
        return 0;
    case 4:
        snprintf(result, resultlen, "blue");
        return 0;
    case 5:
        snprintf(result, resultlen, "magenta");
        return 0;
    case 6:
        snprintf(result, resultlen, "cyan");
        return 0;
    case 7:
        snprintf(result, resultlen, "white");
        return 0;
    case 9:
        snprintf(result, resultlen, "default");
        return 0;
    default:
        snprintf(result, resultlen, "unknown");
        return 1;
    }
}

int process_controlsequence(char final_byte, char *intermediate_bytes, char *parameter_bytes)
{
    char buffer[BUFFER_SIZE];

    switch (final_byte) {
    case 0x48: {
        int row = 1, col = 1;
        if (*parameter_bytes != 0) {
            int len = 0;
            while (*parameter_bytes != ';' && *parameter_bytes != 0) buffer[len++] = *parameter_bytes++;
            buffer[len++] = 0;
            row = ascii_to_dec(buffer, &len);
            col = 1;
            if (*parameter_bytes == ';') {
                parameter_bytes++;
                len = 0;
                while (*parameter_bytes != 0) buffer[len++] = *parameter_bytes++;
                buffer[len++] = 0;
                col = ascii_to_dec(buffer, &len);
            }
        }
        printf("Moving cursor to position row=%d, column=%d\n", row, col);
        fprintf(xmloutputfile, "<cursor absoluterow=\"%d\" absolutecolumn=\"%d\" />\n", row, col);
    }
    return 0;
    case 0x4a: {
        /// ED -- Erase in Page (see 8.3.39 in ECMA-48 1991)
        int len = 1;
        int param = ascii_to_dec(parameter_bytes, &len);
        if (len == 0) {
            /// No parameter provided, assuming default value param=0
            param = 0;
            len = 1;
        }

        if (len == 1) {
            printf("Control Sequence: Erase in Page (param=%d)\n", param);
            fprintf(xmloutputfile, "<erase scope=\"in_page\" range=\"%s\" />\n", param == 0 ? "cur_to_end" : (param == 1 ? "begin_to_cur" : "all"));
        } else {
            printf("Invalid len: %d\n", len);
            return 1;
        }
    }
    return 0;
    case 0x4b: {
        /// EL -- Erase in Line (see 8.3.41 in ECMA-48 1991)
        int len = 1;
        int param = ascii_to_dec(parameter_bytes, &len);
        if (len == 0) {
            /// No parameter provided, assuming default value param=0
            param = 0;
            len = 1;
        }

        if (len == 1) {
            printf("Control Sequence: Erase in Page (param=%d)\n", param);
            fprintf(xmloutputfile, "<erase scope=\"in_line\" range=\"%s\" />\n", param == 0 ? "cur_to_end" : (param == 1 ? "begin_to_cur" : "all"));
        } else {
            printf("Invalid len: %d\n", len);
            return 1;
        }
    }
    return 0;
    case 0x68:
        if (intermediate_bytes[0] == 0) {
            /// SM -- Set Mode (see 8.3.125 in ECMA-48 1991)
            printf("Control Sequence: Set Mode (parameter length=%zu, intermediate length=%zu)\n", strlen(parameter_bytes), strlen(intermediate_bytes));

            int dec_mode = 0;
            if (*parameter_bytes == 0x3f) {
                // DEC Private Mode Set (DECSET)
                ++parameter_bytes;
                dec_mode = 1;
            } else {
                // ANSI mode
            }

            int parameters[ARRAY_LENGTH];
            int parameters_len = parameterstring_to_intarray(parameter_bytes, BUFFER_SIZE, parameters, ARRAY_LENGTH);

            if (parameters_len == 1 && parameters[0] == 1) {
                printf("Application takes over control of cursor keys\n");
                fprintf(xmloutputfile, "<cursor key-control=\"application\" />\n");
            } else if (parameters_len == 1 && parameters[0] == 12) {
                printf("Start blinking cursor\n");
                fprintf(xmloutputfile, "<cursor blinking=\"true\" />\n");
            } else if (parameters_len == 1 && parameters[0] == 25) {
                printf("Hide cursor cursor\n");
                fprintf(xmloutputfile, "<cursor show=\"false\" />\n");
            } else if (parameters_len == 1 && (parameters[0] == 47 || parameters[0] == 1047 || parameters[0] == 1049)) {
                printf("Switching to alternate screen\n");
                if (parameters[0] == 1049)
                    fprintf(xmloutputfile, "<cursor state=\"save\" />\n");
                fprintf(xmloutputfile, "<screen switchto=\"1\" />\n");
            } else if (parameters_len == 1 && parameters[0] == 1034) {
                printf("Interpret \"meta\" key, sets eighth bit\n");
                fprintf(xmloutputfile, "<special state=\"8bit\" />\n");
            } else if (parameters_len == 1 && parameters[0] == 1048) {
                fprintf(xmloutputfile, "<cursor state=\"save\" />\n");
            } else {
                printf("dec_mode=%d\n", dec_mode);
                printf("parameters_len=%d\n", parameters_len);
                for (int i = 0; i < parameters_len; ++i)
                    printf("parameters[%d]=%d\n", i, parameters[i]);
            }

            return 0;
        } else {
            printf("Unsupported Control Sequence that ends with 0x68\n");
            return 0;
        }
    case 0x6c:
        /// RM -- Reset Mode (see 8.3.106 in ECMA-48 1991)
        printf("Control Sequence: Reset Mode (parameter length=%zu, intermediate length=%zu)\n", strlen(parameter_bytes), strlen(intermediate_bytes));

        int dec_mode = 0;
        if (*parameter_bytes == 0x3f) {
            // DEC Private Mode Set (DECSET)
            ++parameter_bytes;
            dec_mode = 1;
        } else {
            // ANSI mode
        }

        int parameters[ARRAY_LENGTH];
        int parameters_len = parameterstring_to_intarray(parameter_bytes, BUFFER_SIZE, parameters, ARRAY_LENGTH);
        if (parameters_len == 1 && parameters[0] == 1) {
            printf("Terminal takes over control of cursor keys\n");
            fprintf(xmloutputfile, "<cursor key-control=\"terminal\" />\n");
        } else if (parameters_len == 1 && parameters[0] == 12) {
            printf("Stop blinking cursor\n");
            fprintf(xmloutputfile, "<cursor blinking=\"false\" />\n");
        } else if (parameters_len == 1 && parameters[0] == 25) {
            printf("Show cursor cursor\n");
            fprintf(xmloutputfile, "<cursor show=\"true\" />\n");
        } else if (parameters_len == 1 && (parameters[0] == 47 || parameters[0] == 1047 || parameters[0] == 1049)) {
            printf("Switching back from alternate screen\n");
            if (parameters[0] == 1049)
                fprintf(xmloutputfile, "<cursor state=\"restore\" />\n");
            fprintf(xmloutputfile, "<screen switchto=\"0\" />\n");
        } else if (parameters_len == 1 && parameters[0] == 1048) {
            fprintf(xmloutputfile, "<cursor state=\"restore\" />\n");
        } else {
            printf("dec_mode=%d\n", dec_mode);
            printf("parameters_len=%d\n", parameters_len);
            for (int i = 0; i < parameters_len; ++i)
                printf("parameters[%d]=%d\n", i, parameters[i]);
        }

        return 0;
    case 0x6d:
        /// SGR -- Select Graphics Rendition (see 8.3.117 in ECMA-48 1991)
        printf("Control Sequence: Detected color change (parameter length=%zu, intermediate length=%zu)\n", strlen(parameter_bytes), strlen(intermediate_bytes));

        int intense = 0, faint = 0;

        while (strlen(parameter_bytes) >= 2) {
            int len = 2;
            int color = ascii_to_dec(parameter_bytes, &len);
            if (len != 2) break;
            if (color == 0) {
                printf("Resetting colors\n");
                fprintf(xmloutputfile, "<color operation=\"reset\" />\n");
                intense = 0;
                faint = 0;
            } else if (color == 1) {
                printf("Using intense colors\n");
                intense = 1;
                faint = 0;
            } else if (color == 2) {
                printf("Using faint colors\n");
                intense = 0;
                faint = 1;
            } else if ((color >= 30 && color <= 37) || color == 39) {
                char colorstring[BUFFER_SIZE];
                colortostring(color, colorstring, BUFFER_SIZE);
                printf("Foreground using color \"%s\" (%i)\n", colorstring, color);
                fprintf(xmloutputfile, "<color foreground=\"%s-%s\" />\n", intense == 0 ? (faint == 0 ? "normal" : "faint") : "intense", colorstring);
            } else if (color == 38) {
                printf("Future unsupported foreground color\n");
                fprintf(xmloutputfile, "<color foreground=\"normal-default\" />\n");
                break;
            } else if ((color >= 40 && color <= 47) || color == 49) {
                char colorstring[BUFFER_SIZE];
                colortostring(color, colorstring, BUFFER_SIZE);
                printf("Background using color \"%s\" (%i)\n", colorstring, color);
                fprintf(xmloutputfile, "<color background=\"%s-%s\" />\n", intense == 0 ? (faint == 0 ? "normal" : "faint") : "intense", colorstring);
            } else if (color == 48) {
                printf("Future unsupported background color\n");
                fprintf(xmloutputfile, "<color background=\"normal-default\" />\n");
            } else {
                printf("Unknown color code: %u\n", color);
                fprintf(xmloutputfile, "<color operation=\"reset\" />\n");
            }
            if (parameter_bytes[2] == ';')
                parameter_bytes += 3;
            else
                break;
        }

        return 0;
    case 0x6e: {
        /// DSR -- Device Status Report (see 8.3.35 in ECMA-48 1991)
        int len = 1;
        int param = ascii_to_dec(parameter_bytes, &len);
        if (len == 0) {
            /// No parameter provided, assuming default value param=0
            param = 0;
            len = 1;
        }

        if (len == 1) {
            printf("Control Sequence: Device Status Report (param=%d)\n", param);
        } else {
            printf("Invalid len: %d\n", len);
            return 1;
        }
    }
    return 0;
    default:
        printf("Don't know Final Byte 0x%02x for Control Sequence (parameter length=%zu, intermediate length=%zu)\n", final_byte, strlen(parameter_bytes), strlen(intermediate_bytes));
        return 0;
    }
}

/**
 * @param expected_size Number of bytes describing the event of the current step
 */
int process_typescript_step(size_t expected_size)
{
    char csi_parameter_bytes[BUFFER_SIZE];
    char csi_intermediate_bytes[BUFFER_SIZE];
    char osc_string[BUFFER_SIZE];
    char csi_final_byte;
    int ret = 0;
    int insidetextsequence = 0;

    if (expected_size > typescriptbuffer_size) {
        /// The current typescript buffer is too small.
        /// Release and allocate a larger memory region.
        free(typescriptbuffer);
        typescriptbuffer_size = roundup_powerof2(expected_size + 2);
        typescriptbuffer = (char *)calloc(typescriptbuffer_size, sizeof(char));
    }

    /// Read as many bytes from the typescript files as are expected
    /// to describe the current step's events
    size_t rlen = fread(typescriptbuffer, sizeof(char), expected_size, typescriptfile);
    if (rlen < expected_size) {
        fprintf(stderr, "Expected to read %zu bytes from typescript file, got only %zu\n", expected_size, rlen);
        return 1;
    }

    /// Go through every byte in the typescript buffer ...
    for (size_t i = 0; ret == 0 && i < rlen; ++i) {
        if (typescriptbuffer[i] == 0x0a) {
            printf("char: Line Feed  (%zu of %zu)\n", i, rlen - 1);
            if (insidetextsequence == 1) {
                /// If open, close current <text> environment
                fprintf(xmloutputfile, "</text>\n");
                insidetextsequence = 0;
            }
            fprintf(xmloutputfile, "<newline />\n");
        } else if (typescriptbuffer[i] == 0x0d) {
            printf("char: Carriage Return  (%zu of %zu)\n", i, rlen - 1);
            if (insidetextsequence == 1) {
                /// If open, close current <text> environment
                fprintf(xmloutputfile, "</text>\n");
                insidetextsequence = 0;
            }
            if (i < rlen - 1 && typescriptbuffer[i + 1] != 0x0a) ///< lonely CR without following LF
                fprintf(xmloutputfile, "<newline />\n");
        } else if (typescriptbuffer[i] >= 32 && typescriptbuffer[i] < 128) {
            printf("char: %c  (%zu of %zu)\n", typescriptbuffer[i], i, rlen - 1);
            if (insidetextsequence == 0) {
                /// If not open, open a <text> environment
                fprintf(xmloutputfile, "<text>");
                insidetextsequence = 1;
            }
            /// Handle XML entities correctly
            xmlized_print(xmloutputfile, typescriptbuffer[i]);
        } else if (typescriptbuffer[i] == 0x1b /* ESCAPE */ && i < rlen - 1 /* and more bytes follow */) {
            if (insidetextsequence == 1) {
                /// If open, close current <text> environment
                fprintf(xmloutputfile, "</text>\n");
                insidetextsequence = 0;
            }

            /// Escape sequence
            if (typescriptbuffer[i + 1] == 0x5b /* 05/11 from 7-bit C1 set */) {
                /// CSI -- Command Sequence Introducer (see 5.4 in ECMA-48 1991)
                printf("CSI at position %zu of %zu\n", i, rlen - 1);
                i += 2;

                size_t sequence_len;
                /// Reading optional Parameter Bytes that have to be in value range 0x30 .. 0x3f
                sequence_len = 0;
                while (sequence_len < BUFFER_SIZE - 1 && i < rlen && typescriptbuffer[i] >= 0x30 && typescriptbuffer[i] <= 0x3f) {
                    csi_parameter_bytes[sequence_len++] = typescriptbuffer[i++];
                }
                csi_parameter_bytes[sequence_len++] = 0; ///< null-terminated
                /// Reading optional Intermediate Bytes that have to be in value range 0x20 .. 0x2f
                sequence_len = 0;
                while (sequence_len < BUFFER_SIZE - 1 && i < rlen && typescriptbuffer[i] >= 0x20 && typescriptbuffer[i] <= 0x2f)  {
                    csi_intermediate_bytes[sequence_len++] = typescriptbuffer[i++];
                }
                csi_intermediate_bytes[sequence_len++] = 0;
                /// Reading Final Byte that has to be in value range 0x40 .. 0x7f
                if (i < rlen && typescriptbuffer[i] >= 0x40 && typescriptbuffer[i] <= 0x7f) {
                    /// Found Final Byte
                    csi_final_byte = typescriptbuffer[i++];

                    ret = process_controlsequence(csi_final_byte, csi_intermediate_bytes, csi_parameter_bytes);
                    --i; /// Compensate for for-loop's ++i
                } else {
                    printf("Final Byte expected at position %zu of %zu, but byte 0x%02x found instead\n", i, rlen - 1, typescriptbuffer[i]);
                }
            } else if (typescriptbuffer[i + 1] == 0x5d /* 5/13 from 7-bit C1 set */) {
                /// OSC -- Operating System Command (see 8.3.89 in ECMA-48 1991)
                printf("OSC at position %zu of %zu\n", i, rlen - 1);
                i += 2;

                /// Read command string
                size_t osc_string_len = 0;
                while (i < rlen && osc_string_len < BUFFER_SIZE - 1 && ((typescriptbuffer[i] >= 0x08 /* 00/08 */ && typescriptbuffer[i] <= 0x0d /* 00/13 */) || (typescriptbuffer[i] >= 0x20 /* 02/00 */ && typescriptbuffer[i] <= 0x7e /* 07/14 */)))
                    osc_string[osc_string_len++] = typescriptbuffer[i++];
                osc_string[osc_string_len] = '\0';
                if (osc_string_len > 3 && osc_string[0] == '0' && osc_string[1] == ';') {
                    /// OSC starting with '0;' sets the window title, the remaining
                    /// printable characters are the title

                    if (insidetextsequence == 1) {
                        /// If open, close current <text> environment
                        fprintf(xmloutputfile, "</text>\n");
                        insidetextsequence = 0;
                    }
                    printf("Window title=");
                    fprintf(xmloutputfile, "<osc type=\"windowtitle\">");
                    for (size_t j = 2; j < osc_string_len; ++j)
                        if (osc_string[j] >= 0x20 /* 02/00 */ && osc_string[j] <= 0x7e /* 07/14 */) {
                            /// Write out printable characters only
                            printf("%c", osc_string[j]);
                            /// Handle XML entities correctly
                            xmlized_print(xmloutputfile, osc_string[j]);
                        }
                    printf("\n");
                    fprintf(xmloutputfile, "</osc>\n");
                } else {
                    printf("unknown command string=");
                    for (size_t j = 0; j < osc_string_len; ++j) {
                        if (osc_string[j] >= 0x20 /* 02/00 */ && osc_string[j] <= 0x7e /* 07/14 */)
                            printf("%c", osc_string[j]);
                        else
                            printf("[%02x]", osc_string[j]);
                    }
                    printf("\n");
                }

                /// Read String Terminator
                if (i < rlen && typescriptbuffer[i] == 0x9c) {
                    /// 8-bit single-byte String Terminator (see 8.3.143 in ECMA-48 1991)
                    ++i;

                    --i; /// Compensate for for-loop's ++i
                } else if (i < rlen - 1 && typescriptbuffer[i] == 0x1b && typescriptbuffer[i + 1] == 0x5c) {
                    /// 7-bit double-byte String Terminator (see 8.3.143 in ECMA-48 1991)
                    i += 2;

                    --i; /// Compensate for for-loop's ++i
                } else if (i < rlen && typescriptbuffer[i] == 0x07) {
                    /// Sometimes a BEL is acceptable as an alternative to a String Terminator
                    ++i;

                    --i; /// Compensate for for-loop's ++i
                } else if (i < rlen - 1) {
                    /// Bytes left to read but no valid String Terminator
                    printf("String Terminator expected at position %zu of %zu, but byte 0x%02x found instead\n", i, rlen - 1,  typescriptbuffer[i]);
                }
            } else if (typescriptbuffer[i + 1] >= 64 /* 04/00 */ && typescriptbuffer[i + 1] <= 95 /* 05/15 */) {
                printf("Unknown 7-bit 2-byte escape sequence from C1 set: %02x %02x\n", typescriptbuffer[i + 1], typescriptbuffer[i + 2]);
                /// 2-byte sequence
                i += 2;
                --i; /// Compensate for for-loop's ++i
            } else if (typescriptbuffer[i + 1] >= 128 /* 08/00 */ && typescriptbuffer[i + 1] <= 159 /* 09/15 */) {
                printf("Unknown 8-bit 2-byte escape sequence from C1 set: %02x %02x\n", typescriptbuffer[i + 1], typescriptbuffer[i + 2]);
                /// 2-byte sequence
                i += 2;
                --i; /// Compensate for for-loop's ++i
            } else if (typescriptbuffer[i + 1] >= 0x3c /* 03/12 */ && typescriptbuffer[i + 1] <= 0x3f /* 03/15 */) {
                printf("Private parameter string: %c%c\n", typescriptbuffer[i + 1], typescriptbuffer[i + 2]);
                /// Assuming 2-byte sequence
                i += 2;
                --i; /// Compensate for for-loop's ++i
            } else {
                fprintf(stdout, "Unknown escape sequence: 0x%02x='%c' at position %zu of %zu\n", typescriptbuffer[i + 1], typescriptbuffer[i + 1], i, rlen - 1);
                /// Assuming 2-byte sequence
                i += 2;
                --i; /// Compensate for for-loop's ++i
            }
        } else {
            if (insidetextsequence == 1) {
                fprintf(xmloutputfile, "</text>\n");
                insidetextsequence = 0;
            }

            if (typescriptbuffer[i] & 0x80)
                printf("8-bit char: 0x%02x  (%zu of %zu)\n", typescriptbuffer[i] & 0xff, i, rlen - 1);
            else
                printf("char: 0x%02x  (%zu of %zu)\n", typescriptbuffer[i], i, rlen - 1);
        }
    }

    if (insidetextsequence == 1) {
        /// If open, close current <text> environment
        fprintf(xmloutputfile, "</text>\n");
        insidetextsequence = 0;
    }

    return ret;
}

int process_timefile()
{
    /// Ignore the first typescript line, contains just a comment
    skipline(typescriptfile);

    /// The timing file is line-based. In each line, there are
    /// two fields: A time stamp representing the delay since the
    /// previous line and a positive integer number representing
    /// how many bytes are to be read from the typescript file

    for (int line_nr = 0; ; ++line_nr) {
        double delay;
        size_t blk;
        char nl;
        if (fscanf(timefile, "%lf %zu%c\n", &delay, &blk, &nl) != 3 ||
                nl != '\n') {
            if (feof(timefile))
                break;
            if (ferror(timefile)) {
                fprintf(stderr, "Error while reading timimg file: unexpected format in line %d\n", line_nr);
                return 2;
            }
        }

        fprintf(xmloutputfile, "<timestep delay=\"%.3f\">\n", delay);

        int ret = process_typescript_step(blk);
        if (ret != 0)
            return ret;

        fprintf(xmloutputfile, "</timestep>\n");
    }

    return 0;
}

int main(int argc, char *argv[])
{
    /// Require three parameters passed to this program.
    if (argc != 4) {
        printf("Require three parameters: timefilename typescriptfilename xmloutputfilename, got %d parameters\n", argc - 1);
        return 1;
    }

    char *timefilename = argv[argc - 3];
    timefile = fopen(timefilename, "r");
    if (!timefile) {
        printf("Cannot open timefilename \"%s\"\n", timefilename);
        return 1;
    }

    char *typescriptfilename = argv[argc - 2];
    typescriptfile = fopen(typescriptfilename, "r");
    if (!typescriptfile) {
        fclose(timefile);
        printf("Cannot open typescriptfilename \"%s\"\n", typescriptfilename);
        return 1;
    }

    char *xmloutputfilename = argv[argc - 1];
    xmloutputfile = fopen(xmloutputfilename, "w");
    if (!xmloutputfile) {
        fclose(typescriptfile);
        fclose(timefile);
        printf("Cannot open xmloutputfilename \"%s\"\n", xmloutputfilename);
        return 1;
    } else {
        fprintf(xmloutputfile, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
        fprintf(xmloutputfile, "<script>\n");
    }

    /// Initial size of buffer typescriptbuffer, will grow later
    typescriptbuffer_size = 16;
    typescriptbuffer = (char *)calloc(typescriptbuffer_size, sizeof(char));

    int ret = process_timefile();
    if (ret != 0) {
        fclose(xmloutputfile);
        fclose(timefile);
        fclose(typescriptfile);
        free(typescriptbuffer);
        return ret;
    }

    fprintf(xmloutputfile, "</script>\n");

    fclose(xmloutputfile);
    fclose(timefile);
    fclose(typescriptfile);
    free(typescriptbuffer);

    return 0;
}
