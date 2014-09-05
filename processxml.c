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

#define _POSIX_SOURCE

#include <stdio.h>
#include <stdio.h>
#include <string.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#define BUFFER_SIZE 16384

int debug_output;
double accumulated_delay;

xmlNode *parse_timestep_node(xmlNode *timestepnode) {
    if (timestepnode->type != XML_ELEMENT_NODE) {
        fprintf(stderr, "Current node \"%s\" is not an element\n", timestepnode->name);
        return timestepnode->next;
    } else if (xmlStrEqual(timestepnode->name, (xmlChar *)"script")) {
        fprintf(stderr, "Current node's name is not \"timestep\" but \"%s\"\n", timestepnode->name);
        return timestepnode->next;
    }

    if (timestepnode->children != NULL &&  timestepnode->children->type == XML_TEXT_NODE &&  timestepnode->children->next == NULL && timestepnode->children->content[0] <= 32 && timestepnode->children->content[1] <= 32) {
        /// Just a single text element with white space
        xmlAttr *curAttr = timestepnode->properties;
        while (curAttr != NULL) {
            if (xmlStrEqual(curAttr->name, (xmlChar *)"delay")) {
                double this_delay = atof((const char *)curAttr->children->content);
                accumulated_delay += this_delay;
                xmlNode *next = timestepnode->next;
                xmlUnlinkNode(timestepnode);
                xmlFreeNode(timestepnode);

                /// Remove following empty-line text node
                if (next->type == XML_TEXT_NODE && next->content[0] <= 32) {
                    xmlNode *next2 = next->next;
                    xmlUnlinkNode(next);
                    xmlFreeNode(next);
                    next = next2;
                }

                return next;
            }
            curAttr = curAttr->next;
        }
    }

    if (accumulated_delay > 0.0) {
        xmlAttr *curAttr = timestepnode->properties;
        while (curAttr != NULL) {
            if (xmlStrEqual(curAttr->name, (xmlChar *)"delay")) {
                double this_delay = atof((const char *)curAttr->children->content);
                xmlFreeNode(curAttr->children);
                char printed_delay[BUFFER_SIZE];
                snprintf(printed_delay, BUFFER_SIZE, "%.3lf", accumulated_delay + this_delay);
                accumulated_delay = 0.0;
                curAttr->children = xmlNewText((unsigned char *)printed_delay);
                break;
            }
            curAttr = curAttr->next;
        }
    }

    // TODO

    return timestepnode->next;
}

int parse_script_node(xmlNode *scriptnode) {
    if (scriptnode->type != XML_ELEMENT_NODE) {
        fprintf(stderr, "Current node \"%s\" is not an element\n", scriptnode->name);
        return 4;
    } else if (xmlStrEqual(scriptnode->name, (xmlChar *)"script") == 0) {
        fprintf(stderr, "Current node's name is not \"script\" but \"%s\"\n", scriptnode->name);
        return 4;
    }

    for (xmlNode *cur = scriptnode->children; cur; /** cur is stepped forward below */) {
        if (cur->type == XML_ELEMENT_NODE && xmlStrEqual(cur->name, (xmlChar *)"timestep"))
            cur = parse_timestep_node(cur);
        else
            cur = cur->next;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    debug_output = 0;
    char *inputfilename = NULL;
    char *outputfilename = NULL;
    FILE *inputfile = NULL;
    FILE *outputfile = NULL;

    LIBXML_TEST_VERSION;

    if (argc > 1 && strcmp("--debug", argv[1]) == 0) {
        fprintf(stderr, "Enabling debug output\n");
        debug_output = 1;
    }

    if ((argc > 1 && debug_output == 0) || argc > 2) {
        inputfilename = argv[1 + debug_output];
        if (debug_output) fprintf(stderr, "Reading XML from file \"%s\"\n", inputfilename);
    } else if (debug_output)
        fprintf(stderr, "Reading XML from stdin\n");

    if ((argc > 2 && debug_output == 0) || argc > 3) {
        outputfilename = argv[2 + debug_output];
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

    xmlDocPtr doc = xmlReadFd(fileno(inputfile), inputfilename == NULL || inputfilename[0] == '\0' ? "noname.xml" : inputfilename, NULL, 0);
    if (doc == NULL) {
        if (inputfilename != NULL)
            fprintf(stderr, "Failed to parse \"%s\"\n", inputfilename);
        if (outputfile != stdout)
            fclose(outputfile);
        if (inputfile != stdin)
            fclose(inputfile);
        return 1;
    }
    xmlNode *root_element = xmlDocGetRootElement(doc);

    accumulated_delay = 0.0;
    int result = parse_script_node(root_element);

    if (result == 0)
        result = xmlDocDump(outputfile, doc) > 0 ? 0 : 1;

    /// Free the XML document
    xmlFreeDoc(doc);

    if (outputfile != stdout)
        fclose(outputfile);
    if (inputfile != stdin)
        fclose(inputfile);

    // Cleanup function for the XML library.
    xmlCleanupParser();

    return result;
}
