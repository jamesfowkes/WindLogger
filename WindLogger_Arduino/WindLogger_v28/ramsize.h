#ifndef _RAMSIZE_H_
#define _RAMSIZE_H_

#ifndef RAMSTART

#ifdef __AVR_ATmega328P__
#define RAMSTART 0x100
#endif

#endif

#define RAMSIZE (RAMEND-RAMSTART+1)

#endif

