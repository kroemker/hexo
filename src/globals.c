
#include "globals.h"

unsigned int readInt(unsigned char* data, unsigned int pos)
{
    unsigned int ret = (unsigned int)data[pos] << 24;
    ret += (unsigned int)data[pos+1] << 16;
    ret += (unsigned int)data[pos+2] << 8;
    ret += (unsigned int)data[pos+3];
    return ret;
}

unsigned int hexCharToInteger(char hc)
{
    switch(hc)
    {
    case '0':
        return 0;
    case '1':
        return 1;
    case '2':
        return 2;
    case '3':
        return 3;
    case '4':
        return 4;
    case '5':
        return 5;
    case '6':
        return 6;
    case '7':
        return 7;
    case '8':
        return 8;
    case '9':
        return 9;
    case 'a':
        return 0xa;
    case 'b':
        return 0xb;
    case 'c':
        return 0xc;
    case 'd':
        return 0xd;
    case 'e':
        return 0xe;
    case 'f':
        return 0xf;
    case 'A':
        return 0xa;
    case 'B':
        return 0xb;
    case 'C':
        return 0xc;
    case 'D':
        return 0xd;
    case 'E':
        return 0xe;
    case 'F':
        return 0xf;
    }
    return 0;
}
