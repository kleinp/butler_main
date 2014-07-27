/* basicIO.h
 * Peter Klein
 * Created on April 21, 2013, 11:05 AM
 * Description:
 *
 */

#ifndef BASICIO_H
#define	BASICIO_H

// Flags
#define PAD_RIGHT           1
#define PAD_ZERO            2
#define ALWAYS_SHOW         4
// Modifiers
#define UNSIGNED            1
#define HEXLC               2
#define HEXUC               4

#define PRINT_BUF_LEN       11

char *strTok(char *string, const char *del);
int strSpn(const char *string, const char *set);
char *strPbrk(const char *string, const char *set);
int printF(const char *format, ...);
int sPrintF(char *out, const char *format, ...);
void printDec(int x);

#endif	/* BASICIO_H */

