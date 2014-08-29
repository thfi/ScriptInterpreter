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

#include "utils.h"

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

/**
 * Write a character to an output file.
 * Replace special characters like '&' or '<' by
 * their escaped form like '&amp;' or '&lt;'.
 */
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
