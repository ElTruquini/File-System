#ifndef DISKINFO_H_
#define DISKINFO_H_

extern unsigned int bsize, bcount, fatstart, fatblocks, rootstart, rootblocks;

extern void intializeVars(unsigned char** waka);

#endif