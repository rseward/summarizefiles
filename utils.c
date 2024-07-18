
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


char *strtrim(char *str)
{
    // Find the start of non-whitespace characters
    char *start = str + strspn(str, " \t\r\n");

    // Find the end of non-whitespace characters (excluding null terminator)
    char *end = str + strlen(str) - 1;
    while (end >= start && isspace(*end))
        {
            end--;
        }

    // Shift non-whitespace characters to the beginning of the string
    if (end >= start)
        {
            end++;  // Include the null terminator
            memmove(str, start, end - start);
            str[end - start] = '\0';  // Null terminate the trimmed string
        }
    else
        {
            // Handle empty string case (all whitespace)
            *str = '\0';
        }

    return str;
}

int mysystem(char *strbuf, char *cmd, int buffer_size)
{
    FILE *fp;
    int bytes_read;

    // Open pipe for reading the command output
    fp = popen(cmd, "r");
    if (fp == NULL)
        {
            perror("popen");
            exit(EXIT_FAILURE);
        }

    // Read data from the pipe until EOF
    while ((bytes_read = fread(strbuf, 1, buffer_size - 1, fp)) > 0)
        {
            strbuf[bytes_read] = '\0';  // Null terminate the string
            printf("%s", strbuf);
        }

    // Check for errors during read
    if (ferror(fp) != 0)
        {
            perror("fread");
            pclose(fp);
            exit(EXIT_FAILURE);
        }

    // Close the pipe and wait for child process to finish
    int status = pclose(fp);
    if (status == -1)
        {
            perror("pclose");
            exit(EXIT_FAILURE);
        }

    // Check the exit status of the child process (optional)
    if (WIFEXITED(status))
        {
            printf("Command exited with status: %d\n", WEXITSTATUS(status));
        }
    else
        {
            printf("Command terminated abnormally\n");
        }

    return status;
}

char* substr(const char* str, int start, int length, char *sbuf)
{
    if (sbuf == NULL)
        {
            return NULL;  // Handle memory allocation failure
        }
    if (start==-1)
        {
            // compute start to capture the last of the string
            start=strlen(str)-length;
        }
    strncpy(sbuf, str + start, length);  // Copy the substring
    sbuf[length] = '\0';  // Ensure null termination
    return sbuf;
}


