/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   virOSFS_UI.h
 * Author: root
 *
 * Created on 16 April 2019, 6:20 PM
 */

#ifndef VIROSFS_UI_H
#define VIROSFS_UI_H

#include "virOSFS.h"
#include "fontworker.h"
#include "virOS.h"

void echo(char * cmd);
void _ls(void);
void _ls(void *p, ...);
void _ll(void);

void _ls(void)
{
    echo("ls\n\0");
    update_FDir();
    uint8_t idx, cnt = 0, fcnt = 0;
    //print_str("\n\0");
    for (idx = 0; idx < FILE_COUNT_MAX; idx++) {
        //convert code to filename
        uint8_t filename_code;
        filename_code = fDir.filenames[idx];
        if ((filename_code & 0x80) > 0) {
            fcnt++;
            char * fname;
            fname = "..\0";
            fname[0] = 0x61 + ((0x7F & filename_code) >> 3); //ascii "a" is 0x61
            fname[1] = 0x31 + (0x07 & filename_code); //ascii "1" is 0x31

            if (cnt > 7) {
                cnt = 0;
                print_str("\n\0");
            }
            print_str(fname);
            pFS->position.x = pFS->cursor.x;
            cnt++;
        }
    }
    print_str("\n\0");
    if (fcnt == 1) printfi_str(get_virOS_message(virOS_FS_UI_1_FILE), fcnt);
    else printfi_str(get_virOS_message(virOS_FS_UI_NUM_FILES), fcnt);

}

void _ls(void *p, ...)
{
    va_list ap;
    va_start(ap, p);
    va_end(ap);
}

void _ll(void)
{
    echo("ll\n\0");
    update_FDir();
    uint8_t idx, fcnt = 0;
    //print_str("\n\0");
    for (idx = 0; idx < FILE_COUNT_MAX; idx++) {
        FDI fdi = fetch_fdi(idx);
        char * fname;
        fname = "..\0";
        if (fdi.file_exists) {
            fcnt++;
            fname[0] = 0x61 + fdi.name_letter; //ascii "a" is 0x61
            fname[1] = 0x31 + fdi.name_number; //ascii "1" is 0x31
            pFS->position.x = 0;
            // pFS->position.y += (pFS->font_pixel_h + pFS->font_pixel_vpad) * pFS->font_size;
            print_int(idx);
            pFS->position.x = pFS->cursor.x;
            print_str(" \0");
            pFS->position.x = pFS->cursor.x;
            print_str(fname);
            pFS->position.x = pFS->cursor.x;
            print_str(" \0");
            pFS->position.x = pFS->cursor.x;
            print_int(fdi.size_LOWER | (fdi.size_UPPER << 6));
            print_str("\n\0");
        }
    }
    //pFS->position.x = 0;
    //pFS->position.y += (pFS->font_pixel_h + pFS->font_pixel_vpad) * pFS->font_size;
    if (fcnt == 1) printfi_str(get_virOS_message(virOS_FS_UI_1_FILE), fcnt);
    else printfi_str(get_virOS_message(virOS_FS_UI_NUM_FILES), fcnt);
}

void echo(char* cmd)
{
    print_str(cmd);
}

#endif /* VIROSFS_UI_H */

