/* 
 * This file is part of the simple_can (https://github.com/davidevansg/simple_can_tx).
 * Copyright (c) 2020 David Evans.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*********** Includes *************/
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <pthread.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>

#include <string.h>
#include <sys/ioctl.h>
#include <sys/timerfd.h>

#include "defs.h"
#include "can.h"
#include "main.h"

/*********** Static Functions ***********/
static uint8_t CANIfcInit(void);
static uint8_t CANThrInit(void);

static uint8_t AssembleFrame(struct can_frame *fra);
static uint8_t SendFrame(struct can_frame *fra);

static void PrintCANFrame(struct can_frame *fra);
static void tick_tx();

/*********** Static Variables ***********/
static int can_fd;

uint8_t reason_to_quit = 0;
/*********** Global Variables ***********/
pthread_t thr_tick;

/************************************************************
 * Function Name Thr_1msTick
 *
 * Purpose: Simple 1ms tick function
 *
 * \param[in] void*ptr
 *
 * \return
 *      void
 ***********************************************************
 */
void *Thr_Tick(void *ptr)
{
    int timer_fd = timerfd_create(CLOCK_REALTIME, 0);
    unsigned long long missed;
    struct itimerspec itval;

    itval.it_interval.tv_sec = 0;
    itval.it_interval.tv_nsec = (100 * (NS_TO_MS));
    itval.it_value.tv_sec = itval.it_interval.tv_sec;
    itval.it_value.tv_nsec = itval.it_interval.tv_nsec;
    int timer_status = timerfd_settime(timer_fd, 0, &itval, NULL);

    /* While the timer is OK and we have no reason to  */
    while ((timer_status != -1) && (reason_to_quit != 1) )
    {
        timer_status = read(timer_fd, &missed, sizeof(missed));
        if (timer_status < 0)
        {
            printf("breaking\n"); 
            break;
        }

        tick_tx();
    }

    printf("out of loop\n");
    CANClose();

    return NULL;
}

/************************************************************
 * Function Name tick_tx
 *
 * Purpose: Calls the respective functionality every
 *          'tick' (100ms in this application).
 *
 * \param[in] void
 *
 * \return void
 *
 ***********************************************************
 */
static void tick_tx()
{
    // 1ms tick
    struct can_frame frame;
    uint8_t status;
    uint8_t ret;
    if(AssembleFrame(&frame) != SUCCESS)
    {
        return;
    }
    if(SendFrame (&frame) != SUCCESS)
    {
        return;
    }
    PrintCANFrame(&frame);
}


/************************************************************
 * Function Name AssembleFrame
 *
 * Purpose: Assembles a simple CAN frame.
 *
 * \param[in] *fra
 *      A pointer to can_frame structure (see /linux/can.h)
 *
 * \return
 *      SUCCESS if frame has been appropriately set
 *      FAILURE if there's an issue with the input can
 *          frame structure
 *
 ***********************************************************
 */
static uint8_t AssembleFrame(struct can_frame *fra)
{
    static uint8_t sequence = 0;

    if(fra != NULL)
    {
        (*fra).can_id = 0x100;
        (*fra).can_dlc = 0x08;
        memset(&(*fra).data[0], 0x00, (*fra).can_dlc);
        (*fra).data[0] = sequence++;
        return SUCCESS;
    }
    else
    {
        printf("fra null\n");
        return FAILURE;
    }
}

/************************************************************
 * Function Name SendFrame
 *
 * Purpose: Sends a simple CAN frame.
 *
 * \param[in] *fra
 *      A pointer to can_frame structure (see /linux/can.h)
 *
 * \return
 *      SUCCESS if there's no reason to believe there was
 *          an issue sending the CAN frame.
 *      FAILURE if there was an issue sending the frame.
 *
 ***********************************************************
 */
static uint8_t SendFrame(struct can_frame *fra)
{
    uint8_t size = SUCCESS;
    if(fra != NULL)
    {
        size = send(can_fd, fra, sizeof(struct can_frame), MSG_DONTWAIT);
        if (size != sizeof(struct can_frame))
        {
            printf("Problem writing frame\nIs your CAN interface 'up'\n");
            reason_to_quit = 1;
            return FAILURE;
        }
        return SUCCESS;
    }
    else
    {
        return FAILURE;
    }
}

/************************************************************
 * Function Name PrintCANFrame
 *
 * Purpose: Prints the contents of a given CAN frame.
 *
 * \param[in] *fra
 *      A pointer to can_frame structure (see /linux/can.h)
 *
 * \return void
 *
 ***********************************************************
 */
static void PrintCANFrame(struct can_frame *fra)
{
    if(fra != NULL)
    {
        printf("ID = %04x ", (*fra).can_id);
        printf("DLC = [%02x] - ", (*fra).can_dlc);
        for(int i = 0; i < (*fra).can_dlc; i++)
        {
            printf("B%d|%02x ", i, (*fra).data[i]);
        }
        printf("\n");
    }
    else
    {
        printf("fra null\n");
    }
}

/************************************************************
 * Function Name CANThrInit
 *
 * Purpose: Initialise a 1ms thread.
 *
 * \param[in] void
 *
 * \return uint8_t
 *      SUCCESS if successfully created, otherwise
 *      FAILURE
 ***********************************************************
 */
static uint8_t CANThrInit()
{
    if (pthread_create(&thr_tick, NULL, Thr_Tick, NULL))
    {
        perror("Error creating thread\n");
        return FAILURE;
    }

    return SUCCESS;
}

/************************************************************
 * Function Name 
 *
 * Purpose: Initialises the CAN interface and sets up
 *          the correpsonding options
 *
 * \param[in] void
 *
 * \return
 *      SUCCESS if successfully created, otherwise
 *      FAILURE
 ***********************************************************
 */
static uint8_t CANIfcInit(void)
{
    struct ifreq ifr;
    struct sockaddr_can addr;

    /* Create a socket */
    can_fd = socket(AF_CAN, SOCK_RAW, CAN_RAW);
    if (can_fd < 0)
    {
        printf("Cannot create socket, quitting\n");
        return FAILURE;
    }
    strcpy(ifr.ifr_name, IF_VCAN0);

    /* Associate socket with interface (e.g. "vcan0" */
    if(ioctl(can_fd, SIOCGIFINDEX, &ifr) < 0)
    {
        printf("Cannot link socket with interface - does the interface exist?\n");
        return FAILURE;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(can_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("Unable to bind socket, quitting\n");
        return FAILURE;
    }

    return SUCCESS;
}


/************************************************************
 * Function Name ISOTPCANClose
 *
 * Purpose: Tidy up, we're shutting down
 *
 * \param[in] void
 *
 * \return
 *      SUCCESS
 ***********************************************************
 */
uint8_t CANClose(void)
{
    close(can_fd);
    reason_to_quit = 1;
    Shutdown();

    return SUCCESS;
}

/************************************************************
 * Function Name ISOTPCANInit
 *
 * Purpose: High level CAN initialisation function, sets
 *          everything up from main.c
 *
 * \param[in] void
 *
 * \return
 *      SUCCESS if successfully initialised, otherwise
 *      FAILURE
 ***********************************************************
 */
uint8_t CANInit(void)
{
    if( (CANIfcInit()  == SUCCESS) &&
        (CANThrInit()  == SUCCESS) )
    {
        printf("CAN Init Success\n");
        return SUCCESS;
    }
    else
    {
        printf("close\n");
        CANClose();
        return FAILURE;
    }
}
