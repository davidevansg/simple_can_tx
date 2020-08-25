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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>

#include "main.h"
#include "defs.h"
#include "can.h"

/*********** Static Functions ***********/
/*********** Static Variables ***********/
/*********** Global Variables ***********/
uint8_t app_active = 1;
uint8_t can_active = 1;

/************************************************************
 * Function Name Shutdown
 *
 * Purpose: If we're in difficulty, a high level shutdown
 *          function to stop the program.
 *
 * \param[in] void
 *
 * \return
 *      void
 ***********************************************************
 */
void Shutdown()
{
    can_active = 0;
}

/************************************************************
 * Function Name terminate
 *
 * Purpose: handle ctrl-c signal kill.
 *
 * \param[in] void
 *
 * \return
 *      void
 ***********************************************************
 */
void terminate(int param)
{
    printf("killing thread\n");
    app_active = 0;
    CANClose();
}

/************************************************************
 * Function Name main
 *
 * Purpose: main entry point for the application + loop
 *          to keep it alive.
 *
 * \param[in] void
 *
 * \return
 *      0 - when done.
 ***********************************************************
 */
int main(int argc, char *argv[])
{
    void (*sig)(int);
    sig = signal(SIGINT, terminate);

    if  (CANInit() == SUCCESS)
    {
        // Everything is running
    }

    while ((app_active) && (can_active) )
    {
        usleep(ONE_SEC);
    }

    printf("quiting...\n");
    return 0;
}

