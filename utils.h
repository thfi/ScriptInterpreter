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

#ifndef SCRIPTINTERPRETER_UTILS_H
#define SCRIPTINTERPRETER_UTILS_H

/**
 * For a given integer number n, return
 * - 1 if n is zero or negative
 * - n itself if n is a power of 2
 * - the next power of 2 larger than n
 */
int roundup_powerof2(int n);

/**
 * Continue reading from an input file,
 * discarding all read data until
 * - there is a line break denoted by \n
 * - the end of file is reached
 */
void skipline(FILE *input);

/**
 * Write a character to an output file.
 * Replace special characters like '&' or '<' by
 * their escaped form like '&amp;' or '&lt;'.
 */
void xmlized_print(FILE *output, char c);

/**
 * Convert a sequence of characters into an integer number.
 * The sequence is limited by 'len' or the first appearance
 * of a (semi)colon or the null character.
 * The function will convert a valid sequence like '6721' to
 * the numeric value 6721.
 * The function will return -1 if the sequence contains an
 * invalid character like 'a' or 'x'.
 */
int ascii_to_dec(char *sequence, int *len);

#endif // SCRIPTINTERPRETER_UTILS_H
