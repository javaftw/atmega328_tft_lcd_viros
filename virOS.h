/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   virOS.h
 * Author: root
 *
 * Created on 13 April 2019, 1:30 PM
 */

#ifndef VIROS_H
#define VIROS_H

#include "virOSFS.h"

/********************************************************** Internal Opcodes */
#define IOC_NOP             0x00

/********************************************************** Memory Management */

#define virOS_RESERVED_MEMORY_SIZE          0x3F
uint8_t RESERVED_MEMORY_BLOCK[virOS_RESERVED_MEMORY_SIZE];

/******************************************************* Tasks and Handlers */

typedef enum {
    TASK_TYPE_GPIO,
    TASK_TYPE_DATA,
    TASK_TYPE_USART,
    TASK_TYPE_I2C,
    TASK_TYPE_SPI,
    TASK_TYPE_ADC,
    TASK_TYPE_PWM,
    TASK_TYPE_FILE_SYSTEM,
    TASK_TYPE_MEMORY_MANAGEMENT,
    TASK_TYPE_POWER_MANAGEMEN
} TaskType;

typedef enum {
    TASK_STATE_WAIT, //no cpu time, not ready to run
    TASK_STATE_READY, //no cpu time, ready to run
    TASK_STATE_RUNNING, //has cpu time, is running, is interruptable
    TASK_STATE_BLOCKED//has cpu time, is running, not interruptable
} TaskState;

typedef struct {
    const uint8_t UID;
    TaskState state;
    uint16_t ISR_return_vector;
    uint16_t PC_; //copy of PC before entering task
    uint16_t SP_; //copy of SP before entering task
    uint8_t * stack;
} Task;

typedef enum {
    HANDLER_TYPE_INTERRUPT,
    HANDLER_TYPE_CYCLIC,
    HANDLER_TYPE_EXCEPTION
} HandlerType;

/***************************************************************** Scheduling */


//=============================================== Utility
//=============================================== Messages

const char virOS_FS_INSTALL_COMPLETE[] PROGMEM = "virOSFS Installed\n\0";
const char intro_1[] PROGMEM = "-= virOS =- v 0.15\n\0";
const char intro_2[] PROGMEM = "-------------------\n\0";
const char intro_3[] PROGMEM = "virOSFS v%d\n\0";
const char virOS_FEC_ALREADY_EXISTS[] PROGMEM = "*FE:File exists\n\0";
const char virOS_FS_UI_NUM_FILES[] PROGMEM = "%d files\n\0";
const char virOS_FS_UI_1_FILE[] PROGMEM = "%d file\n\0";
const char virOS_FS_UI_FILE_WRITTEN[] PROGMEM = "Wrote %d bytes\n\0";


PGM_P const virOS_FEC_MESSAGES[] PROGMEM = {
    intro_1,
    intro_2,
    intro_3,
    virOS_FS_INSTALL_COMPLETE,
    virOS_FEC_ALREADY_EXISTS,
    virOS_FS_UI_NUM_FILES,
    virOS_FS_UI_1_FILE
};

const char * get_virOS_message(PGM_P);

const char * get_virOS_message(PGM_P arr_name)
{
    static char buffer[24];
    strcpy_P(buffer, arr_name);
    return buffer;
}



#endif /* VIROS_H */

