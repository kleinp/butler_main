/* mmri.c
 * Peter Klein
 * Created on November 3, 2013, 6:50 PM
 * Description:
 *
 */

#include "system.h"
#include "mmri.h"
#include "uart.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "nvm.h"

// Memory map structure
MMRIreg MMRI[MMRI_NUM];

/*
 * Address  Description                            Type     R/W   Default Value
 * -----------------------------------------------------------------------------
 * 0        Programmer defined tag                 STRING   0     "FW + DATE"
 * 1        User defined tag                       STRING   1     ""
 * 2        Password                               STRING   1     "abc123"
 * 3        Sysconfig                              UINT8    1     0x00
 *           - write 0x01 to reset
 *           - write 0x02 to save registers to NVM
 * 4        Uart 1 baurate                         UINT32   1     460800
 *
 */

// System registers
char *programmer_tag = "";
char user_tag[20] = "";
char password[20] = "abc123";
uint8_t mmri_config = 0;
uint32_t uart_baud = 921600;

// Global variables
uint8_t permission_level = 0;
uint8_t nvm[256];
char rw_stat[] = "ro";
char *type[] = {"undef ", "uint8 ", "int8  ", "uint16", "int16 ", "uint32",
                "int32 ", "uint64", "int64 ", "float ", "string"};

/*******************************************************************************
 * Function:      mmriInit
 * Inputs:        None
 * Outputs:       None
 * Description:   This function initializes the MMRI structure, populating
 *                system registers, and reading data from NVM, if applicable
 * ****************************************************************************/
void mmriInit()
{
   uint16_t i;
   // Clear the memory
   for(i=0;i<MMRI_NUM;i++)
   {
      MMRI[i].used = 0;
      MMRI[i].type = UNDEF;
   }

   printf("\nINFO: %s\n", build_info);

   mmriInitVar(0, STRING, RO, VOL, NOPW, programmer_tag);
   mmriInitVar(1, STRING, RW, NVM, NOPW, &user_tag[0]);
   mmriInitVar(2, STRING, RO, VOL, NOPW, build_info);
   mmriInitVar(3, STRING, RW, NVM, PWRD, &password[0]);
   mmriInitVar(4, UINT8, RW, VOL, NOPW, &mmri_config);
   mmriInitVar(5, UINT32, RW, NVM, PWWR, &uart_baud);

}

/*******************************************************************************
 * Function:      mmriSaveNVM
 * Inputs:        None
 * Outputs:       None
 * Description:   This function takes an initialized MMRI structure and saves
 *                applicable variables (the ones tagged as NVM) to the EEPROM
 * ****************************************************************************/
void mmriSaveNVM()
{
   uint16_t i, j;
   uint16_t addr = 0;
   uint8_t *str_ptr;
   int64Bytes value;

   // Initialize the NVM array
   for(i=0;i<255;i++)
      nvm[i] = 0;

   // Go through the list of MMRI variables
   // and save off the ones that have been tagged as NVM
   for(i=0;i<MMRI_NUM;i++)
   {
      if (addr > 255)
      {
         printf("\nEEPROM FULL!\n");
         return;
      }
      if (MMRI[i].nvm && MMRI[i].used)
      {
         switch(MMRI[i].type)
         {
            case UINT8:
            case INT8:
               value.num = (uint64_t)*(uint8_t*)MMRI[i].ptr;
               nvm[addr++] = value.bytes[0];
               break;
            case UINT16:
            case INT16:
               value.num = (uint64_t)*(uint16_t*)MMRI[i].ptr;
               nvm[addr++] = value.bytes[0];
               nvm[addr++] = value.bytes[1];
               break;
            case UINT32:
            case INT32:
               value.num = (uint64_t)*(uint32_t*)MMRI[i].ptr;
               for(j=0;j<4;j++)
                  nvm[addr++] = value.bytes[i];
               break;
            case UINT64:
            case INT64:
               value.num = (uint64_t)*(uint64_t*)MMRI[i].ptr;
               for(j=0;j<8;j++)
                  nvm[addr++] = value.bytes[i];
               break;
            case FLOAT:
               value.num = (uint64_t)*(float*)MMRI[i].ptr;
               for(j=0;j<4;j++)
                  nvm[addr++] = value.bytes[i];
               break;
            case STRING:
               str_ptr = (uint8_t*)MMRI[i].ptr;
               for(j=0;j<20;j++)
                  nvm[addr++] = *str_ptr++;
               break;
            default:
               break;
         }
      }
   }
   // Write the entire array to NVM
   nvmWriteArray(0, 256, &nvm[0]);
}

/*******************************************************************************
 * Function:      mmriReadNVM
 * Inputs:        None
 * Outputs:       None
 * Description:   This function reads the EEPROM and puts the values into the
 *                MMRI structure. The assumption is that the MMRI structure is
 *                the same as when the NVM was written, otherwise it will load
 *                incorrectly!
 *                NOTE: Should only be called after all MMRI variables that are
 *                      non-volatile have been initialized!
 * ****************************************************************************/
void mmriReadNVM()
{
   uint16_t i, j;
   uint16_t addr = 0;
   uint8_t *str_ptr;
   int64Bytes value;

   // Load EEPROM into memory
   nvmReadArray(0, 256, &nvm[0]);

   for(i=0;i<MMRI_NUM;i++)
   {
      if (MMRI[i].nvm && MMRI[i].used)
      {
         switch(MMRI[i].type)
         {
            case UINT8:
            case INT8:
               value.bytes[0] = nvm[addr++];
               *(uint8_t*)MMRI[i].ptr = (uint8_t)value.num;
               break;
            case UINT16:
            case INT16:
               value.bytes[0] = nvm[addr++];
               value.bytes[1] = nvm[addr++];
               *(uint16_t*)MMRI[i].ptr = (uint16_t)value.num;
               break;
            case UINT32:
            case INT32:
               for(j=0;j<4;j++)
                  value.bytes[j] = nvm[addr++];
               *(uint32_t*)MMRI[i].ptr = (uint32_t)value.num;
               break;
            case UINT64:
            case INT64:
               for(j=0;j<8;j++)
                  value.bytes[j] = nvm[addr++];
               *(uint64_t*)MMRI[i].ptr = (uint64_t)value.num;
               break;
            case FLOAT:
               for(j=0;j<4;j++)
                  value.bytes[j] = nvm[addr++];
               *(float*)MMRI[i].ptr = (float)value.num;
               break;
            case STRING:
               str_ptr = MMRI[i].ptr;
               for(j=0;j<20;j++)
                  *str_ptr++ = nvm[addr++];
               break;
            default:
               break;
         }
      }
   }

   // If we are loading from an erased or uninitialized EEPROM
   // set the password to 'A'
   if (password[0] == 0 || password[0] == 0xFF)
   {
      password[0] = 'A';
      password[1] = '\0';
   }
}

/*******************************************************************************
 * Function:      mmriInitVar
 * Inputs:        <uint8 addr> the register address to use. 0-3 are used by the
 *                             system, so only 4-255 are available to the user
 *                <uint8 type> the type of data type will be put here
 *                             (UINT8, INT8, UINT16, INT16, UINT32, INT32,
 *                              FLOAT, STRING)
 *                <uint8 rw> read-only (RO) or read-write (RW) by the user
 *                <uint8 nvm> (NVM) if the parameter should be stored in
 *                            non-volatile memory. If not, (VOL)atile
 *                <uint8 pwp> (NOPW) to allow anyone to write the register
 *                            (PWWR) to require privilege to write
 *                            (PWRD) to require privilege to read/write
 *                <void *ptr> A pointer to the data. Data does not need to be
 *                            a local variable
 * Outputs:       None
 * Description:   Initializes a single register will all of its possible options
 * ****************************************************************************/
void mmriInitVar(uint8_t addr, uint8_t type, uint8_t rw, uint8_t nvm,
                 uint8_t pwp, void *ptr)
{
   MMRI[addr].used = 1;
   MMRI[addr].rw = rw;
   MMRI[addr].type = type;
   MMRI[addr].pwp = pwp;
   MMRI[addr].nvm = nvm;
   MMRI[addr].ptr = ptr;
}

/*******************************************************************************
 * Function:      mmriGetRegPtr
 * Inputs:        <uint8 addr> address of the register to read
 * Outputs:       <void *ptr> pointer to the register value
 * Description:   Returns the pointer of the memory location the register
 *                value starts at. Is a way for any file to easily access
 *                a mmri registered variable without having to include the file
 * ****************************************************************************/
void *mmriGetRegPtr(uint8_t addr)
{
   return (MMRI[addr].ptr);
}

/*******************************************************************************
 * Function:      mmriPrintReg
 * Inputs:        <uint8 addr> address of the register to read
 * Outputs:       None
 * Description:   Prints the value associated with the register address
 *                provided. If the user does not have password privileges, it
 *                does not display pasword read protected registers (PWRD), but
 *                5 '*' characters instead
 * ****************************************************************************/
void mmriPrintReg(uint8_t addr)
{
   if ((permission_level + 1) >= MMRI[addr].pwp)
   {
      switch(MMRI[addr].type)
      {
         case UINT8:    printf("%u", *(uint8_t*)MMRI[addr].ptr);     break;
         case INT8:     printf("%i", *(int8_t*)MMRI[addr].ptr);      break;
         case UINT16:   printf("%u", *(uint16_t*)MMRI[addr].ptr);    break;
         case INT16:    printf("%i", *(int16_t*)MMRI[addr].ptr);     break;
         case UINT32:   printf("%lu", *(uint32_t*)MMRI[addr].ptr);   break;
         case INT32:    printf("%li", *(int32_t*)MMRI[addr].ptr);    break;
         case UINT64:   printf("%llu", *(uint64_t*)MMRI[addr].ptr);  break;
         case INT64:    printf("%lli", *(int64_t*)MMRI[addr].ptr);   break;
         case FLOAT:    printf("%f", *(double*)MMRI[addr].ptr);      break;
         case STRING:   printf("%s", (char*)MMRI[addr].ptr);         break;
         default:       printf("?");                                 break;
      }
   }
   else
      printf("*****");
}

/*******************************************************************************
 * Function:      mmriWriteRegAscii
 * Inputs:        <uint8 addr> address of the register to write
 *                <char *value> value in ASCII string to write to register
 * Outputs:       <uint8> error code, or 0 if none
 * Description:   This function will write the value given by the ASCII string
 *                to the register address. Simple checks performed for correct
 *                data type. Also handles special register, such as password
 *                unlock/lock and config registers
 * ****************************************************************************/
uint8_t mmriWriteRegAscii(uint8_t addr, char *value)
{
   char *val = value;
   char *end;
   uint8_t tmp;
   uint32_t unsigned_val;
   int32_t signed_val;
   //uint64_t unsigned_ll_val;
   //int64_t signed_ll_val;
   float float_val;

   // special case, register 2 - password
   if (addr == 2)
   {
      if (permission_level)   // if we already have permission, write new password
      {
         strcpy(&password[0], val);
         return(NOERROR);
      }
      else               // if not, compare the value given and stored password
      {
         if (strcmp(&password[0], val) == 0)
         {
            permission_level = 2;   // give privileges if they match
            return(NOERROR);
         }
         else
            return(BADPASS);
      }
   }
   // special case register 3 - MMRI config
   else if (addr == 3)
   {
      tmp = (uint8_t)atoi(val);
      switch(tmp)
      {
         case 1:                 // reset device
            reset();
            break;
         case 2:                 // Save register values to NVM
            if (permission_level > 1)
            {
               mmriSaveNVM();
               return(NOERROR);
            }
            else
            {
               return(BADPASS);
            }
         case 3:                 // undo password permission level
            permission_level = 0;
            return(NOERROR);
         case 4:                 // Print all variables
            mmriPrintAllReg();
            return(NOERROR);
         case 5:                 // Zero out NVM
            if (permission_level > 1)
            {
               for(unsigned_val=0;unsigned_val<255;unsigned_val++)
                  nvm[unsigned_val] = 0;
               nvmWriteArray(0, 256, &nvm[0]);
               return(NOERROR);
            }
            else
            {
               return(BADPASS);
            }
         default:
            return(BADVAL);
      }
   }

   // Check that user has permission to write to the register
   // The above 2 registers were special cases
   if (permission_level < MMRI[addr].pwp)
   {
      return(BADPASS);
   }

   if (addr == 4)
   {
      uart_baud = (uint32_t)atoi(val);
      uChangeBaud(uart_baud, U1);
   }
   else if (MMRI[addr].rw && MMRI[addr].used)   // the address is available and writable
   {
      switch(MMRI[addr].type)
      {
         case UINT8:
            unsigned_val = strtoul(val, &end, 0);
            if (unsigned_val > UCHAR_MAX)
               return(BADVAL);
            if (*end != '\0')
               return(BADVAL);
            *(uint8_t*)MMRI[addr].ptr = (uint8_t)unsigned_val;
            return(NOERROR);
         case INT8:
            signed_val = strtol(val, &end, 0);
            if (signed_val > CHAR_MAX || signed_val < CHAR_MIN)
               return(BADVAL);
            if (*end != '\0')
               return(BADVAL);
            *(int8_t*)MMRI[addr].ptr = (int8_t)signed_val;
            return(NOERROR);
         case UINT16:
            unsigned_val = strtoul(val, &end, 0);
            if (unsigned_val > UINT_MAX)
               return(BADVAL);
            if (*end != '\0')
               return(BADVAL);
            *(uint16_t*)MMRI[addr].ptr = (uint16_t)unsigned_val;
            return(NOERROR);
         case INT16:
            signed_val = strtol(val, &end, 0);
            if (signed_val > INT_MAX || signed_val < INT_MIN)
               return(BADVAL);
            if (*end != '\0')
               return(BADVAL);
            *(int16_t*)MMRI[addr].ptr = (int16_t)signed_val;
            return(NOERROR);
         case UINT32:
            unsigned_val = strtoul(val, &end, 0);
            if (*end != '\0')
               return(BADVAL);
            *(uint32_t*)MMRI[addr].ptr = unsigned_val;
            return(NOERROR);
         case INT32:
            signed_val = strtol(val, &end, 0);
            if (*end != '\0')
               return(BADVAL);
            *(int32_t*)MMRI[addr].ptr = signed_val;
            return(NOERROR);
         case UINT64:
            //if (sscanf(val, "%llu", &unsigned_ll_val) != 1) !! XC1.20 doesn't work with ll
            //   return(BADVAL);
            unsigned_val = strtoul(val, &end, 0);
            if (*end != '\0')
               return(BADVAL);
            *(uint64_t*)MMRI[addr].ptr = unsigned_val;
            return(NOERROR);
         case INT64:
            //if (sscanf(val, "%lli", &signed_ll_val) != 1)   !! XC1.20 doesn't work with ll
            //   return(BADVAL);
            signed_val = strtol(val, &end, 0);
            if (*end != '\0')
               return(BADVAL);
            *(int64_t*)MMRI[addr].ptr = signed_val;
            return(NOERROR);
         case FLOAT:
            float_val = strtod(val, &end);
            if (*end != '\0')
               return(BADVAL);
            *(float*)MMRI[addr].ptr = float_val;
            return(NOERROR);
         case STRING:
            if (strlen(val) > 20)
               return(BADVAL);
            strcpy((char*)MMRI[addr].ptr, val);
            return(NOERROR);
         default:
            return(UNKNOWN);
      }
   }
   else
      return(BADADDR);

   return(UNKNOWN);     // should never get here
}

/*******************************************************************************
 * Function:      mmriPrintAllReg
 * Inputs:        None
 * Outputs:       None
 * Description:   Prints a table of all known register along with the selected
 *                options for that register. This function should mainly be used
 *                for program debugging
 * ****************************************************************************/
void mmriPrintAllReg(void)
{
   uint16_t i;
   printf("\n| ADR | RW | N | P | TYPE   | VALUE");
   for(i=0;i<MMRI_NUM;i++)
   {
      if (MMRI[i].used)
      {
         if (MMRI[i].rw)      // change 'rw' flag
            rw_stat[1] = 'w';
         else
            rw_stat[1] = 'o';

         printf("\n| %03i | %s | %i | %i | %s | ", i, &rw_stat[0], MMRI[i].nvm,
               MMRI[i].pwp, type[MMRI[i].type]);
         mmriPrintReg(i);
      }
   }
}

