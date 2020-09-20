/*  isystem.c: Intel System Configuration Device

    Copyright (c) 2020, William A. Beech

        Permission is hereby granted, free of charge, to any person obtaining a
        copy of this software and associated documentation files (the "Software"),
        to deal in the Software without restriction, including without limitation
        the rights to use, copy, modify, merge, publish, distribute, sublicense,
        and/or sell copies of the Software, and to permit persons to whom the
        Software is furnished to do so, subject to the following conditions:

        The above copyright notice and this permission notice shall be included in
        all copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
        IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
        WILLIAM A. BEECH BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
        IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
        CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

        Except as contained in this notice, the name of William A. Beech shall not be
        used in advertising or otherwise to promote the sale, use or other dealings
        in this Software without prior written authorization from William A. Beech.

    MODIFICATIONS:

        11 Sep 20 - Original file.

    NOTES:


*/

#include "system_defs.h"                /* system header in system dir */

//option board types

#define SBC_064     128
#define SBC_464     129
#define SBC_201     130
#define SBC_202     131
#define SBC_204     132
#define SBC_206     133
#define SBC_208     134
#define ZX200A      135

//Single board computer device types

#define i3214       1
#define i8080       2
#define i8085       3
#define i8251       4
#define i8253       5
#define i8255       6
#define i8259       7
#define IOC_CONT    8
#define IPC_CONT    9
#define MULTI       64
#define EPROM       65
#define RAM         66

//System types

#define MDS_210     1
#define MDS_220     2
#define MDS_225     3
#define MDS_230     4
#define MDS_800     5

#define sys_NAME    "Intel System Configuration Controller"

/* external globals */

extern uint16    PCX;

/* function prototypes */

t_stat sys_reset(DEVICE *dptr);
static const char* sys_desc(DEVICE *dptr);
t_stat sys_show_model (FILE *st, UNIT *uptr, int32 val, CONST void *desc);

/* external function prototypes */

extern uint8 reg_dev(uint8 (*routine)(t_bool, uint8, uint8), uint8, uint8);

/* globals */

int32   cpu_model;

typedef struct device {
    const char *name;
    int num;
    uint16 base[4];
    uint16 size[4];
    } SYS_DEV;

typedef struct system_model {
    const char *name;
    int num;
    SYS_DEV devices[30];
    } SYS_MODEL;

SYS_MODEL models[] = {
    {"MDS-210", 9,
        {{ "I8251",     2,  {0xF4, 0xF6},   0       },
         { "I8353",     1,  0xF0,           0       },
         { "I8255",     2,  {0xE4, 0xE8},   0       },
         { "I8259",     2,  {0xFA, 0xFC},   0       },
         { "EPROM",     1,  0x0000,         0x0FFF  },
         { "RAM",       1,  0x0000,         0x7FFF  },
         { "IOC-CONT",  1,  0xC0,           0       },
         { "IPC-CONT",  1,  0xFF,           0       },
         { "SBC464",    1,  0xA800,         0x47FF  },
         {NULL} } },
    {"MDS-220", 8,
        {{ "I8251",     2,  {0xF4, 0xF6},   0       },
         { "I8353",     1,  0xF0,           0       },
         { "I8255",     2,  {0xE4, 0xE8},   0       },
         { "I8259",     2,  {0xFA, 0xFC},   0       },
         { "EPROM",     1,  0x0000,         0x0FFF  },
         { "RAM",       1,  0x0000,         0x7FFF  },
         { "IOC-CONT",  1,  0xC0,           0       },
         { "IPC-CONT",  1,  0xFF,           0       },
         {NULL} } },
    {"MDS-225", 8,
        {{ "I8251",     2,  {0xF4, 0xF6},   0       },
         { "I8353",     1,  0xF0,           0       },
         { "I8255",     2,  {0xE4, 0xE8},   0       },
         { "I8259",     2,  {0xFA, 0xFC},   0       },
         { "EPROM",     1,  0x0000,         0x0FFF  },
         { "RAM",       1,  0x0000,         0xFFFF  },
         { "IOC-CONT",  1,  0xC0,           0       },
         { "IPC-CONT",  1,  0xFF,           0       },
         {NULL} } },
    {"MDS-230", 9,
        {{ "I8251",     2,  {0xF4, 0xF6},   0       },
         { "I8353",     1,  0xF0,           0       },
         { "I8255",     2,  {0xE4, 0xE8},   0       },
         { "I8259",     2,  {0xFA, 0xFC},   0       },
         { "EPROM",     1,  0x0000,         0x0FFF  },
         { "RAM",       1,  0x0000,         0x7FFF  },
         { "IOC-CONT",  1,  0xC0,           0       },
         { "IPC-CONT",  1,  0xFF,           0       },
         { "SBC064",    1,  0x8000,         0x7FFF  },
         {NULL} } },
    {"MDS-800", 4,
        {{ "I3214",     1,  0xFC                    },
         { "I8251",     2,  0xF4, 0xF6,     0       },
         { "EPROM",     2,  0x0000,         0x00FF  , 0xf800, 0x7ff},
         { "SBC064",    1,  0x0000,         0x7FFF  },
         {NULL} } },
    {NULL }
    };

UNIT sys_unit = { UDATA (NULL, 0, 0) };

REG sys_reg[] = {
    { NULL }
};

MTAB sys_mod[] = {
//    { MTAB_XTD | MTAB_VDV, 0, NULL, "SIZE", &isbc464_set_size,
//        NULL, NULL, "Sets the ROM size for EPROM"               },
//    { MTAB_XTD | MTAB_VDV, 0, NULL, "BASE", &isbc464_set_base,
//        NULL, NULL, "Sets the ROM base for EPROM"               },
    { MTAB_XTD|MTAB_VDV, 0, "MODEL", NULL, NULL, &sys_show_model, NULL, 
        "Parameters" },
    { 0 }
};

DEBTAB sys_debug[] = {
    { "ALL", DEBUG_all },
    { "FLOW", DEBUG_flow },
    { "READ", DEBUG_read },
    { "WRITE", DEBUG_write },
    { "XACK", DEBUG_xack },
    { "LEV1", DEBUG_level1 },
    { "LEV2", DEBUG_level2 },
    { NULL }
};

/* address width is set to 16 bits to use devices in 8086/8088 implementations */

DEVICE sys_dev = {
    "SYSTEM",           //name
    &sys_unit,          //units
    sys_reg,            //registers
    sys_mod,            //modifiers
    1,                  //numunits
    0,                  //aradix
    0,                  //awidth
    0,                  //aincr
    0,                  //dradix
    0,                  //dwidth
    NULL,               //examine
    NULL,               //deposit
    sys_reset,          //reset
    NULL,               //boot
    NULL,               //attach
    NULL,               //detach
    NULL,               //ctxt
    0,                  //flags
    0,                  //dctrl
    sys_debug,          //debflags
    NULL,               //msize
    NULL,               //lname
    NULL,               //help routine
    NULL,               //attach help routine
    NULL,               //help context
    &sys_desc           //device description
};

static const char* sys_desc(DEVICE *dptr) {
    return sys_NAME;
}

t_stat sys_reset(DEVICE *dptr)
{
//    if (dptr != NULL)
//        return SCPE_ARG;
    return SCPE_OK;
}

/* Set/show CPU model */

t_stat sys_set_model (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
    if (cptr != NULL)
        return SCPE_ARG;
//    if (val >= MOD_MAX)
//        return SCPE_IERR;
    if (val == (int32) cpu_model)
        return SCPE_OK;
    cpu_model = val;
    reset_all (0);                                          /* reset world */
    return SCPE_OK;
}

t_stat sys_show_model (FILE *st, UNIT *uptr, int32 val, CONST void *desc)
{
    int i, j;
    int model = 4;
    
    if (uptr == NULL)
        return SCPE_ARG;
    fprintf(st, "%s:\n", models[model].name);
    for (i=0; i<models[model].num; i++) {
        fprintf(st, "  %s:", models[model].devices[i].name);
        fprintf(st, " %d", models[model].devices[i].num);
        for (j=0; j<4; j++) {
            fprintf(st, " 0%04XH", models[model].devices[i].base[j]);
        }
        for (j=0; j<4; j++) {
            fprintf(st, " 0%04XH", models[model].devices[i].size[j]);
        }
        fprintf(st, "\n");
    }
    return SCPE_OK;
}

/* end of isystem.c */
