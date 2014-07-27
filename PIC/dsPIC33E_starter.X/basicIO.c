/* basicIO.c
 * Peter Klein
 * Created on April 21, 2013, 11:03 AM
 * Description: Functions useful for interaction via a UART. Includes a modified
 * very small implementation of printf
 */

/*
   Very small printf source code downloaded from:
   http://www.menie.org/georges/embedded/printf.html

	Copyright 2001, 2002 Georges Menie (www.menie.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "system.h"
#include "basicIO.h"
#include "uart.h"
#include "stdarg.h"

/*******************************************************************************
 * Function:      strTok
 * Inputs:        <char *string> The string to search through
 *                <char *del> Delimiters to seach for
 * Outputs:       <char *string> Pointer to parsed string subset
 * Description:   Splits <string> into smaller strings, separated by delimiters
 *                (<del>). This function keeps track of the string pointed to,
 *                so only the initial call must supply the search <string> and
 *                subsequent calls must use "NULL". Example:
 *
 *                char *message = "This, is a message";
 *                subs = strTok(message, ", ");
 *                while (subs != NULL)
 *                {
 *                   printF("[%s]",subs);
 *                   subs = strTok(NULL, ", ");
 *                }
 *
 *                Should print "[This][is][a][message]"
 * ****************************************************************************/
char *strTok(char *string, const char *del)
{
   static char *last_token = NULL;
   char *tmp;

   if (string == NULL)
   {
      string = last_token;
      if (string == NULL)
         return NULL;
   }
   else
      string += strSpn(string, del);

   tmp = strPbrk(string, del);
   if (tmp)
   {
      *tmp = '\0';
      last_token = tmp+1;
   }
   else
      last_token = NULL;

   return string;
}

/*******************************************************************************
 * Function:      strSpn
 * Inputs:        <char *string> The string to search through
 *                <char *set> The character set allowed
 * Outputs:       <int length> length of where <string> only has <set> characters
 * Description:   Finds the length of the initial segment of <string> that
 *                contain only characters from <set>
 * ****************************************************************************/
int strSpn(const char *string, const char *set)
{
   const char *x;
   int i;

   for(i=0;*string;string++, i++)
   {
      for(x=set; *x; x++)
         if (*string == *x)
            goto continue_outer;
      break;
      continue_outer:;
   }
   return i;
}

/*******************************************************************************
 * Function:      strPbrk
 * Inputs:        <char *string> The string to search through
 *                <char *set> The characters that return a match
 * Outputs:       <char *match> Pointer to the first match of <set> in <string>
 * Description:   Searches through <string> until it finds the first occurance
 *                of any <set> character and returns a pointer to that location
 * ****************************************************************************/
char *strPbrk(const char *string, const char *set)
{
   const char *x;
   for (; *string; string++)
   {
      for(x = set; *x; x++)
         if (*string == *x)
            return (char *)string;
   }
   return NULL;
}

/*******************************************************************************
 * Function:      printChar
 * Inputs:        <char *str> either "STDOUT" (UART1), or a memory location
 *                <char c> The character to print
 * Outputs:       None
 * Description:   printChar either puts the input character into a memory location
 *                to return at a later time, or passes it to uPutChar. UART1
 *                is used in this case. If another UART is desired, modify here
 *                or use sPrintF and then pass pointer..
 * ****************************************************************************/
static void printChar(char **str, char c)
{
	if (str)                // if the str exists (not null)
   {
		**str = c;           // put the data there
		++(*str);
	}
	else
      uPutChar(c,U1);      // send the data to UART
}

/*******************************************************************************
 * Function:      printS
 * Inputs:        <char *out> either "STDOUT" (UART1), or a memory location
 *                <const char *string> The string to print
 *                <int width> Desired width of string to print (will pad)
 *                <int flag>  Applicable flags for strings are '-' and '0'
 * Outputs:       <int> returns number of characters printed
 *                <char **out> Updates this pointer, or prints to "STDOUT"
 * Description:   The printString function. Used by other functions once numbers
 *                have been converted to strings, so it has pad '0' functionality.
 *                Note that if desired width is less than width of string, the
 *                desired width is ignored!
 * ****************************************************************************/
static int printS(char **out, const char *string, int width, int flag)
{
	int pc = 0;
   char padchar = ' ';

	if (width > 0)                      // if desired width is specified
   {
		int len = 0;
		const char *ptr;
		for (ptr = string; *ptr; ++ptr)  // determine length of string to print
         len++;
		if (len >= width)                // if longer than width specified,
         width = 0;                    // ignore the width specified
		else
         width -= len;                 // determine how many pad chars are needed
		if (flag & PAD_ZERO)
         padchar = '0';                // change padchar from ' ' to '0'
	}

	if (!(flag & PAD_RIGHT))            // If padding left (default) print the
   {                                   // padchar now
		for ( ; width > 0; --width)
      {
			printChar (out, padchar);
			pc++;
		}
	}
	for ( ; *string ; ++string)         // print out the intended string
   {
		printChar (out, *string);
		pc++;
	}
	for ( ; width > 0; --width)         // pad remaining width (there won't be
   {                                   // any remaining if already padded left)
		printChar (out, padchar);
		pc++;
	}

	return pc;                          // return number of characters printed
}

/*******************************************************************************
 * Function:      printI
 * Inputs:        <char *out> either "STDOUT" (UART1), or a memory location
 *                <long long i> the variable to print .. up to 64-bit long long
 *                <int width> Desired width of string to print (will pad)
 *                <int flag> Applicable flags are '-', '+', '0'
 *                <int mod> Applicable modifiers are 'u', 'x', and 'X'
 * Outputs:       <int> returns number of characters printed
 *                <char **out> Updates this pointer, or prints to "STDOUT"
 * Description:   Prints whole numbered decimals in base10 or base16 up to
 *                long long taking into account all flags and modifiers.
 * ****************************************************************************/
static int printI(char **out, long long i, int width, int flag, int mod)
{
	char print_buf[PRINT_BUF_LEN];   // 11-bit max length with a +/- sign
	char *s;                         // pointer to above array

   char letbase = 'a';              // Determines if hex is upper or lower case
   unsigned long long base = 10;    // Change base to print in hex
   unsigned long long t, u = i;     // unsigned version of i

   char neg = 0;
	int pc = 0;

   if (mod & (HEXLC | HEXUC))       // if we want to print hex, change base
   {
      base = 16;
      if (mod & HEXUC)              // if also upper case, change ASCII ref
         letbase = 'A';
   }

   // If character to be printed is signed, is base10, and is less than 0,
   // flag to print negative sign later, and print the positive number
	if (!(mod & UNSIGNED) && base == 10 && i < 0)
   {
		neg = 1;
		u = -i;
	}

   // This method goes from back to front, so start with the back, the end-of-line
	s = print_buf + PRINT_BUF_LEN-1;
	*s = '\0';

   
   if (u == 0)                      // special case, u = 0 won't go into                     
      *--s = '0';                   // while loop below

	while (u)
   {
		t = u % base;                 // get the remainder
		if( t >= 10 )                 // if the remainder is greater than 10
			t += letbase - '0' - 10;   // in case of hex, print letters instead
		*--s = t + '0';               // add number to string to print
		u /= base;                    // divide, losing the remainder..
	}

	if (neg)                         // if we were negative (base10 only)
   {                                // print the '-' now if printS will pad
		if(width && (flag & PAD_ZERO))// zeroes, or just add it to the string
      {                             // if we are padding right
			printChar(out, '-');
			pc++;
		}
		else
      {
			*--s = '-';
         width+=1;
      }
	}
   else if (flag & ALWAYS_SHOW)      // In the special case where we always
   {                                // want to show + or -, and number isn't
      if(width && (flag & PAD_ZERO))// negative, show the positive instead!
      {
         printChar(out, '+');
         pc++;
      }
      else
      {
         *--s = '+';
         width+=1;
      }
   }
   
   // print the string we just created
	return pc + printS (out, s, width, flag);
}

static const double rounding[9] = {0.5, 0.05, 0.005, 0.0005, 0.00005,
                                    0.000005, 0.0000005, 0.00000005, 0.000000005};

static const double pow_10[10] = {1, 10, 100, 1000, 10000, 100000, 1000000,
                                 10000000, 100000000, 1000000000};

static int printD(char **out, double f, int width, int precision, int flag)
{
   /* possible flags:
    * PAD_RIGHT      applies to decimal part
    * PAD_ZERO       applies to integer part
    * ALWAYS_SHOW    applies to integer part
    * width          applies to integer part (I'm saying that this is so, not true in normal printf)
    * precision      applies to decimal part
    */
   long long decimal;
   int pc = 0;
   int length = width+precision+1;     // take out the decimal place and sign
   long long integer;

   if (f<0)
      f -= rounding[precision];
   else
      f += rounding[precision];

   integer = (long long)f;
 
   // If the user wants to pad right, don't do any left padding (default)
   // on the integer part. We will pad right after printing the decimal part
   if (flag & PAD_RIGHT)
      width = 0;

   // if the integer number is 0, but the floating point is less than 0, we need
   // to print a negative instead of a positive
   if (integer == 0 && f < 0)
   {
      flag &= 0xFFFB;            // get rid of always show flag
      printChar(out, '-');
   }

   // Print the integer part with the width as specified. Don't allow the
   // pad right option in this case, because this will be applied after the
   // decimal is printed. Also, ignore directive to display as hex or unsigned
   pc += printI(out, integer, width, (flag & 0xFFFE), 0);
   
   // Print the decimal part with the precision specified. This print should
   // always be padded with zero to correctly display the numbers with a leading
   // zero in the decimal.
   if (precision > 0)
   {
      pc += printS(out, ".", 0, 0);
      decimal = (long long)((f - (double)integer)*pow_10[precision]);
      if (decimal < 0)
         decimal = -decimal;
      
      pc += printI(out, decimal, precision, PAD_ZERO, 0);
   }

   // Pad right if necessary, since it can't be done while printing the decimal
   if (flag & PAD_RIGHT && pc < length)
      pc += printS(out, " ", (length-pc), PAD_RIGHT);

   return pc;
}

/*******************************************************************************
 * Function:      print
 * Inputs:        <char **out> either "STDOUT" (UART1), or a memory location
 *                <const char *msg_string> The primary printf string
 *                <va_list va> Additional arguments defined in *msg_string
 * Outputs:       <int> returns number of characters printed
 *                <char **out> Updates this pointer, or prints to "STDOUT"
 * Description:   A small implementation of printf for use on PIC 16-bit
 *                microcontrollers. Modified from online source code:
 *
 *                http://www.menie.org/georges/embedded/printf.html
 *
 *                Send it a string, and variable arguments list..
 *                %[flag][width][modifier][specifier]
 *
 *                NOTE: ORDER OF flags, modifiers matters!
 *
 *                [flag]
 *                -        pad right, instead of default left
 *                0        pad with zeros instead of spaces
 *                +        always show +/- sign, even for positive numbers
 *                [width]
 *                <num>    width to show
 *                [modifier]
 *                x        display in hex, lower case
 *                X        display in hex, UPPER case
 *                u        use the unsigned equivalant of the specifier
 *                [specifier]
 *                s        string         (pointer)
 *                c        char           (8-bit)
 *                i        int            (16-bit)
 *                l        long           (32-bit)
 *                g        long long      (64-bit)
 *                d        double         (32-bit float)
 *                e        long double    (64-bit float) !!NOT IMPLEMENTED!!
 *
 * ****************************************************************************/
static int print(char **out, const char *msg_string, va_list va)
{
   int flag, width, mod, precision;
   int pc = 0;

   while(*msg_string != 0)
   {
      if (*msg_string == '%')       // if the message contains the '%" char...
      {
         msg_string++;
         flag = width = mod = 0;
         precision = 4;
         
         // ***** Special cases ************************************************
         if (*msg_string == '\0')   // end the message
            break;
         if (*msg_string == '%')    // '%' again, print it!
            goto out;

         // ***** Flags ********************************************************
         if (*msg_string == '-')    // pad right modifier
         {
            msg_string++;
            flag = PAD_RIGHT;
         }
         if (*msg_string == '0')    // pad zero modifier
         {
            msg_string++;
            flag |= PAD_ZERO;
         }
         if (*msg_string == '+')    // Always show + symbol
         {
            msg_string++;
            flag |= ALWAYS_SHOW;
         }

         // ***** Width & Precision ********************************************
         while(*msg_string >= '0' && *msg_string <= '9') // get length to show
         {
            width *= 10;
            width += *msg_string - '0';
            msg_string++;
         }
         if (*msg_string == '.') // precision specified. Is ignored for integers
         {                       // only 0-9 decimal places allowed
            msg_string++;
            precision = *msg_string - '0';
            msg_string++;
         }
         // ***** Modifiers ****************************************************
         if (*msg_string == 'x')
         {
            mod |= HEXLC;
            msg_string++;
         }
         if (*msg_string == 'X')
         {
            mod &= 0x01;            // in case HEXLC was set
            mod |= HEXUC;
            msg_string++;
         }
         if (*msg_string == 'u')    // if there is a 'u' modifier, use the
         {                          // unsigned equivalent (i.e. uc = unsigned char)
            mod |= UNSIGNED;
            msg_string++;
         }
         // ***** specifiers ***************************************************
         if (*msg_string == 's')    // print a string
         {
            char *s = (char *)(va_arg (va, char *));
            pc += printS(out, s?s:"(null)", width, flag);
            msg_string++;
            continue;
         }
         if (*msg_string == 'c')    // 8-bit value
         {
            if (mod & UNSIGNED)
               pc += printI(out, (long long)(va_arg (va, unsigned char)), width, flag, mod);
            else
               pc += printI(out, (long long)(va_arg (va, char)), width, flag, mod);
            msg_string++;
            continue;
         }
         if (*msg_string == 'i')    // 16-bit value
         {
            if (mod & UNSIGNED)
               pc += printI(out, (long long)(va_arg (va, unsigned int)), width, flag, mod);
            else
               pc += printI(out, (va_arg (va, int)), width, flag, mod);
            msg_string++;
            continue;
         }
         if (*msg_string == 'l')    // 32-bit value
         {
            if (mod & UNSIGNED)
               pc += printI(out, (long long)(va_arg (va, unsigned long)), width, flag, mod);
            else
               pc += printI(out, (va_arg (va, long)), width, flag, mod);
            msg_string++;
            continue;
         }
         if (*msg_string == 'g')    // 64-bit value
         {
            if (mod & UNSIGNED)
               pc += printI(out, (va_arg (va, unsigned long long)), width, flag, mod);
            else
               pc += printI(out, (va_arg (va, long long)), width, flag, mod);
            msg_string++;
            continue;
         }
         if (*msg_string == 'd')    // 32-bit (floating point) value
         {
            pc += printD(out, (va_arg (va, double)), width, precision, flag);
            msg_string++;
            continue;
         }
         if (*msg_string == 'e')    // 64-bit (floating point) value
         {
            pc += printS(out, "NOT_IMPLEMENTED", width, flag);
            msg_string++;
            continue;
         }

      }
      else
      {
         out:
            printChar (out, *msg_string);
            pc++;
      }

      msg_string++;
   }
   if (out)
      *out = '\0';

   return pc;
}

int printF(const char *format, ...)
{
        va_list args;

        va_start( args, format );
        return print( 0, format, args );
}

int sPrintF(char *out, const char *format, ...)
{
        va_list args;

        va_start( args, format );
        return print( &out, format, args );
}
